/** Version 3 du protocole mictcp :
 *  Garantie de fiabilité partielle « statique » via un mécanisme de reprise des pertes 
 *      de type « Stop and Wait » à fiabilité partielle « pré câblée », 
 *      i.e. dont le % de pertes admissibles est défini de façon statique.
 * 
 *  On envoie donc un message et attend de recevoir l'ack. 
 *      Si il n'est pas recu, on verifie la tolérance et renvoie si besoin. 
 *      Si il est recu et pas conforme à ce attendu, on renvoie systématique le message.
 *  
 *  Par ailleurs, dans cette version, nous ne ferons plus de modulo 2 sur le numero de séquence et d'acquisition.
 *  Cela permet d'éviter l'erreur causé par un saut de message.
 *  Une amélioration de ce programme pourrait etre une remise à zéro des compteurs au bout d'un certain nombre d'envois, pour ne pas épuiser la mémoire.
 */
#include <mictcp.h>
#include <api/mictcp_core.h>

#define LOSS_RATE 60  // En pourcentage, taux de perte fixé
#define TOLERANCE 0.5 // Seuil de tolérance = pertes admises (0=0%; 1=100%)

mic_tcp_sock socket_local; 

int num_sequence=0;
int num_aquisition=0;

double compt_env=0;
double compt_rec=0;

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
    
    /* Encapsulation du message */
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

    /* Attente de l'ACK */
    int sent_size;  // Taille du paquet envoyé
    compt_env++; // Incrémente le compteur d'envois

    while(pdu_ack.header.ack_num!=(num_sequence+1)){ // Regarde le numero de l'ack change ce numéro change => ack reçu

        // Envoi pdu
        sent_size=IP_send(pdu, socket_local.addr);
        if (sent_size==-1){
            printf("Erreur d'envoi \n");
            exit(1);
        }

        // Attente d'expiration du timer ou ack reçu
        if (IP_recv(&pdu_ack, &socket_local.addr, 10)==-1){ // Timer à 10ms
            double perte=1-(compt_rec/compt_env); // Taux de perte = Taux d'echecs
            printf("Timer expiré : paquet perdu \n");
            if (perte>TOLERANCE){
                printf(" ->Perte non tolérée : %f > %f \n", perte, 1-TOLERANCE);
                continue; // On renvoie
            } else {
                printf(" ->Perte tolérée : %f <= %f \n", perte, 1-TOLERANCE);
                break; // On s'arrete ici
            }
        } else if (pdu_ack.header.ack_num==num_sequence+1){ // Ack recu et bonne valeur
            printf("Message correctement envoyé et reçu\n");
            compt_rec++;
        } // Sinon, cela signifie que l'ack recu est non conforme, on reboucle donc
        else {
        printf("Ack recu = %d ", pdu_ack.header.ack_num);
        }
    }
    // Mise à jour du numéro de séquence
    num_sequence=(num_sequence+1);
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
    pdu_ack.payload.size=0; // Ce pdu ne sert qu'a envoyer l'ack, donc pas de payload
    
    // Teste la reception du bon message
    if (pdu.header.seq_num>=num_aquisition){ // Si j'ai reçu le bon message
        app_buffer_put(pdu.payload);
        pdu_ack.header.ack_num=(pdu.header.seq_num+1); // Met à jour l'ack
        num_aquisition=(pdu.header.seq_num+1); // Met à jour le num attendu
    } else {
        pdu_ack.header.ack_num=num_aquisition; // Sinon, met à jour l'ack pour contrer la perte d'ack (j'ai deja recu ce message donc renvoie l'ack d'avant)
    }

    if (IP_send(pdu_ack, socket_local.addr)==-1){// Envoi l'ack
        printf("Erreur dans l'envoi de l'ack \n");
        exit(1);
    }

}
