# Protocole de mictcp :

## Version 1 :
Phase de transfert de données sans garantie de fiabilité 

## Version 2 :
Garantie de fiabilité totale via un mécanisme de reprise des pertes de type « Stop and Wait ».
On envoie donc un message et attend de recevoir l'ack. Si il n'est pas recu ou incorrect, on renvoie. 

## Version 3 :
Garantie de fiabilité partielle « statique » via un mécanisme de reprise des pertes de type « Stop and Wait » à fiabilité partielle « pré câblée », i.e. dont le % de pertes admissibles est défini de façon statique.
On envoie donc un message et attend de recevoir l'ack. Si il n'est pas recu, on verifie la tolérance et renvoie si besoin. Si il est recu et pas conforme à ce attendu, on renvoie systématique le message.

Par ailleurs, dans cette version, nous ne ferons plus de modulo 2 sur le numero de séquence et d'acquisition.
Cela permet d'éviter l'erreur causé par un saut de message.
Une amélioration de ce programme pourrait etre une remise à zéro des compteurs au bout d'un certain nombre d'envois, pour ne pas épuiser la mémoire.


## Commentaires
J'ai mis les trois versions propres dans le dossier mictcp/src/. Il ne faut laisser que celui que l'on veut tester dans le dossier lors du test.