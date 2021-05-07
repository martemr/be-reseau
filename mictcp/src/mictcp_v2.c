/** Version 2 du protocole mictcp :
 *  Garantie de fiabilité totale via un mécanisme de reprise des pertes 
 *      de type « Stop and Wait ». 
 * 
 *  On envoie donc un message et attend de recevoir l'ack. 
 *      Si il n'est pas recu ou incorrect, on renvoie. 
 *  
 */
#include <mictcp.h>
#include <api/mictcp_core.h>
#include <time.h>

#define LOSS_RATE 50  // En pourcentage, taux de perte fixé

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
    set_loss_rate(LOSS_RATE);

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

    // Vérifier qu'on est connecté
    if (socket_local.state!=ESTABLISHED) printf("Erreur : Connection non établie \n");     

    /* Encapsulation */
    mic_tcp_pdu pdu;
        // Header
    pdu.header.seq_num=num_sequence;
    pdu.header.ack_num=num_sequence;
    pdu.header.syn=0;
    pdu.header.ack=0;
    pdu.header.fin=0;
        // Payload
    pdu.payload.data=mesg;
    pdu.payload.size=mesg_size;

    /* Création du pdu servant à recupérer l'ack */
    mic_tcp_pdu pdu_ack;
    pdu_ack.payload.size=0;
    pdu_ack.header.ack_num=num_sequence;

    num_sequence=(num_sequence+1)%2; // Mise à jour du numéro de séquence

    /* Attente de l'ACK */
    int sent_size;
    while(pdu_ack.header.ack_num!=num_sequence){ // Regarde si le numéro reçu est correct

        // Envoi pdu
        sent_size=IP_send(pdu, socket_local.addr);
        if (sent_size==-1) {
            printf("Erreur d'envoi \n");
            exit(1);
        }

        // Attente d'expiration du timer ou ack reçu
        if (IP_recv(&pdu_ack, &socket_local.addr, 10)==-1){
            printf("Paquet perdu, on renvoie \n");
        } else {
            break;
        }
    }

    printf("Message bien envoyé ! \n");

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
    
    /* Créé le pdu qui sera envoyé */
    mic_tcp_pdu pdu_ack;
    pdu_ack.payload.size=0;
    pdu_ack.header.ack_num=pdu.header.ack_num;
    
    // Teste la reception du bon message
    if (pdu.header.seq_num==num_attendu){
        app_buffer_put(pdu.payload);
        num_attendu=(num_attendu+1)%2; // Met à jour le num attendu
        pdu_ack.header.ack_num=(pdu.header.ack_num+1)%2; // Met à jour l'ack
    }

    if (IP_send(pdu_ack, socket_local.addr)==-1){ // Envoi l'ack
        printf("Erreur lors de l'envoi de l'ack \n");
        exit(1);
    }
}
