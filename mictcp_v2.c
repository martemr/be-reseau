#include <mictcp.h>
#include <api/mictcp_core.h>
#include <time.h>

mic_tcp_sock socket_local; 

int num_sequence=0;
int num_attendu=0;

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   
   if(initialize_components(sm)==-1){
       printf("Erreur initialise components \n");
   }
   set_loss_rate(50);

   socket_local.fd=1;
   socket_local.state=IDLE; // Non défini

   return socket_local.fd;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    return 0;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    socket_local.state=ESTABLISHED;
    return 0;
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    
    /* Encapsulation */
    mic_tcp_pdu pdu;
        // Header
    //pdu.header.source_port=socket_local.addr.port;
    //pdu.header.dest_port=adresse_socket.port;
    pdu.header.seq_num=num_sequence;
    pdu.header.ack_num=num_sequence;
    pdu.header.syn=0;
    pdu.header.ack=0;
    pdu.header.fin=0;
        // Payload
    pdu.payload.data=mesg;
    pdu.payload.size=mesg_size;

    // Mise à jour du numéro de séquence
    printf("num_sequence %d \n", num_sequence);
    num_sequence=(num_sequence+1)%2;
    printf("num_sequence %d \n", num_sequence);

    mic_tcp_pdu pdu_ack;
    pdu_ack.payload.size=0;
    pdu_ack.header.ack_num=0;
    
    // Vérifier qu'on est connecté
    if (socket_local.state!=ESTABLISHED) printf("Erreur : Connection non établie \n");     

    /* Attente de l'ACK */
    int sent_size;
    int init=pdu.header.ack_num; // Récupère le numéro de ack initial
    while(pdu.header.ack_num==init){ // Regarde si ce numéro change

        // Envoi pdu
        printf("je vais faire un send \n");
        sent_size=IP_send(pdu, socket_local.addr);
        if (sent_size==-1) return -1;

        // Attente d'expiration du timer ou ack reçu
        if (IP_recv(&pdu, &socket_local.addr, 10)==-1){
            continue; // on renvoie
        }
    }
    return sent_size;
}

/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    
    // Lire le socket 
    mic_tcp_payload payload;
    payload.data = mesg;
    payload.size = max_mesg_size;
    int read_size = app_buffer_get(payload);
    
    return read_size;
}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket)
{
    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    return -1;
}

/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    
    mic_tcp_pdu pdu_ack;
    pdu_ack.payload.size=0;
    
    // Teste la reception du bon message
    if (pdu.header.seq_num==num_attendu){
        app_buffer_put(pdu.payload);
        num_attendu=(num_attendu+1)%2; // Met à jour le num attendu
        pdu_ack.header.ack_num=(pdu.header.ack_num+1)%2; // Met à jour l'ack
    }

    IP_send(pdu_ack, socket_local.addr); // Envoi l'ack

}
