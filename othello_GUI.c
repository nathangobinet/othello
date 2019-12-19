
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <inttypes.h>
#include <arpa/inet.h>

#include <X11/Xlib.h>

#include <gtk/gtk.h>

#include <errno.h>


#define MAXDATASIZE 256


/* Variables globales */
	int damier[8][8];	// tableau associe au damier
	int couleur;		// 0 : pour noir, 1 : pour blanc
	
	int port;		// numero port passe a l'appel

	char *addr_j2, *port_j2;	// Info sur adversaire


	pthread_t thr_id;	// Id du thread fils gerant connexion socket
	
	int sockfd, newsockfd=-1; // descripteurs de socket
	int addr_size;	 // taille adresse
	struct sockaddr *their_addr;	// structure pour stocker adresse adversaire

	fd_set master, read_fds, write_fds;	// ensembles de socket pour toutes les sockets actives avec select
	int fdmax;			// utilise pour select

	//Disable accept while sockfd 
    int disableAccept = 0;

/* Variables globales associées à l'interface graphique */
	GtkBuilder  *  p_builder   = NULL;
	GError      *  p_err       = NULL;
	 


// Entetes des fonctions  
	
/* Fonction permettant de changer l'image d'une case du damier (indiqué par sa colonne et sa ligne) */
void change_img_case(int col, int lig, int couleur_j);

/* Fonction permettant changer nom joueur blanc dans cadre Score */
void set_label_J1(char *texte);

/* Fonction permettant de changer nom joueur noir dans cadre Score */
void set_label_J2(char *texte);

/* Fonction permettant de changer score joueur blanc dans cadre Score */
void set_score_J1(int score);

/* Fonction permettant de récupérer score joueur blanc dans cadre Score */
int get_score_J1(void);

/* Fonction permettant de changer score joueur noir dans cadre Score */
void set_score_J2(int score);

/* Fonction permettant de récupérer score joueur noir dans cadre Score */
int get_score_J2(void);

/* Fonction transformant coordonnees du damier graphique en indexes pour matrice du damier */
void coord_to_indexes(const gchar *coord, int *col, int *lig);

/* Fonction appelee lors du clique sur une case du damier */
static void coup_joueur(GtkWidget *p_case);

/* Fonction retournant texte du champs adresse du serveur de l'interface graphique */
char *lecture_addr_serveur(void);

/* Fonction retournant texte du champs port du serveur de l'interface graphique */
char *lecture_port_serveur(void);

/* Fonction retournant texte du champs login de l'interface graphique */
char *lecture_login(void);

/* Fonction retournant texte du champs adresse du cadre Joueurs de l'interface graphique */
char *lecture_addr_adversaire(void);

/* Fonction retournant texte du champs port du cadre Joueurs de l'interface graphique */
char *lecture_port_adversaire(void);

/* Fonction affichant boite de dialogue si partie gagnee */
void affiche_fenetre_gagne(void);

/* Fonction affichant boite de dialogue si partie perdue */
void affiche_fenetre_perdu(void);

/* Fonction appelee lors du clique du bouton Se connecter */
static void clique_connect_serveur(GtkWidget *b);

/* Fonction desactivant bouton demarrer partie */
void disable_button_start(void);

/* Fonction appelee lors du clique du bouton Demarrer partie */
static void clique_connect_adversaire(GtkWidget *b);

/* Fonction desactivant les cases du damier */
void gele_damier(void);

/* Fonction activant les cases du damier */
void degele_damier(void);

/* Fonction permettant d'initialiser le plateau de jeu */
void init_interface_jeu(void);

/* Fonction reinitialisant la liste des joueurs sur l'interface graphique */
void reset_liste_joueurs(void);

/* Fonction permettant d'ajouter un joueur dans la liste des joueurs sur l'interface graphique */
void affich_joueur(char *login, char *adresse, char *port);



/* Fonction transforme coordonnees du damier graphique en indexes pour matrice du damier */
void coord_to_indexes(const gchar *coord, int *col, int *lig)
{
	char *c;
	
	c=malloc(3*sizeof(char));
	
	c=strncpy(c, coord, 1);
	c[1]='\0';
	
	if(strcmp(c, "A")==0)
	{
		*col=0;
	}
	if(strcmp(c, "B")==0)
	{
		*col=1;
	}
	if(strcmp(c, "C")==0)
	{
		*col=2;
	}
	if(strcmp(c, "D")==0)
	{
		*col=3;
	}
	if(strcmp(c, "E")==0)
	{
		*col=4;
	}
	if(strcmp(c, "F")==0)
	{
		*col=5;
	}
	if(strcmp(c, "G")==0)
	{
		*col=6;
	}
	if(strcmp(c, "H")==0)
	{
		*col=7;
	}
		
	*lig=atoi(coord+1)-1;
}

/* Fonction transforme coordonnees du damier graphique en indexes pour matrice du damier */
void indexes_to_coord(int col, int lig, char *coord)
{
	char c;

	if(col==0)
	{
		c='A';
	}
	if(col==1)
	{
		c='B';
	}
	if(col==2)
	{
		c='C';
	}
	if(col==3)
	{
		c='D';
	}
	if(col==4)
	{
		c='E';
	}
	if(col==5)
	{
		c='F';
	}
	if(col==6)
	{
		c='G';
	}
	if(col==7)
	{
		c='H';
	}
		
	sprintf(coord, "%c%d\0", c, lig+1);
}

/* Fonction permettant de changer l'image d'une case du damier (indiqué par sa colonne et sa ligne) */
void change_img_case(int col, int lig, int couleur_j)
{
	char * coord;
	
	coord=malloc(3*sizeof(char));

	indexes_to_coord(col, lig, coord);
	
	if(couleur_j)
	{ // image pion blanc
		gtk_image_set_from_file(GTK_IMAGE(gtk_builder_get_object(p_builder, coord)), "UI_Glade/case_blanc.png");
	}
	else
	{ // image pion noir
		gtk_image_set_from_file(GTK_IMAGE(gtk_builder_get_object(p_builder, coord)), "UI_Glade/case_noir.png");
	}
}

void partieFinie(score_blanc, score_noire){
	if((score_blanc > score_noire) && couleur == 1) {
		affiche_fenetre_gagne();
	} else if ((score_noire > score_blanc) && couleur == 0) { 
		affiche_fenetre_gagne();
	} else {
		affiche_fenetre_perdu();
	}
}

int estAdjacent(int col, int ligne) {
	if( ((col+1 <= 7) && (damier[col+1][ligne] != -1))
	|| ((col-1 >= 0) && (damier[col-1][ligne] != -1))
	|| ((ligne+1 <= 7) && (damier[col][ligne+1] != -1))
	|| ((ligne-1 >= 0) && (damier[col][ligne-1] != -1))) {
		return 1;
	}
	return 0;
}

int estDansLePlateau(int col, int ligne) {
	if((col <= 7)  && (col >= 0) && (ligne <= 7) && (ligne >= 0)) 
	{
		return 1;
	}
	return 0;
}

void update_damier(int col, int ligne, int couleur_j) {
	damier[col][ligne] = couleur_j;
	change_img_case(col, ligne, couleur_j);
}

void encadrer(col, ligne, couleur_j, deplacementX, deplacementY){
	int index = 1;
	while((estDansLePlateau(col + deplacementX * index, ligne + deplacementY * index) == 1) &&
	(damier[col + deplacementX * index][ligne + deplacementY * index] == (couleur_j ^ 1))) {
		index++;
	}
	if((estDansLePlateau(col + deplacementX * index, ligne + deplacementY * index) == 1) &&
	(damier[col + deplacementX * index][ligne + deplacementY * index] == couleur_j)) {
		for(int i = 0; i < index; i ++) {
			update_damier(col + deplacementX * i,ligne + deplacementY * i, couleur_j);
		}
	}
}


void encadrement(col, ligne, couleur){
	encadrer(col, ligne, couleur, 1 , 0);
	encadrer(col, ligne, couleur, 1 , 1);
	encadrer(col, ligne, couleur, 0 , 1);
	encadrer(col, ligne, couleur, -1 , 0);
	encadrer(col, ligne, couleur, 0 , -1);
	encadrer(col, ligne, couleur, -1 , -1);
	encadrer(col, ligne, couleur, 1 , -1);
	encadrer(col, ligne, couleur, -1 , 1);
}

int get_cases(int* nb_blanc, int* nb_noir)
{
	int nb_vide = 0;
    int i, j;
    for (i = 0; i <= 7; i++)
    {
        for (j = 0; j <= 7; j++)
        {
            if(damier[i][j] == 0){
                *nb_noir = *nb_noir + 1;
            } else if(damier[i][j] == 1){
				*nb_blanc = *nb_blanc + 1;
			} else {
				nb_vide ++;
			}
        }
    }
	return nb_vide;
}

void getScoreAndCheckFinish(){
		int score_blanc = 0;
		int score_noir = 0;
		int nb_vide = get_cases(&score_blanc, &score_noir);

		set_score_J1(score_blanc);
		set_score_J2(score_noir);

		printf("nb_vide : %d, score_blanc: %d, score_noir= %d", nb_vide, score_blanc, score_noir);
		if(nb_vide == 0 || score_blanc == 0 || score_noir == 0) {
			partieFinie(score_blanc, score_noir);
		}
}

int jouer(int col, int ligne, int couleur_j) {
	if(damier[col][ligne] == -1){
		if(estAdjacent(col, ligne) == 1) {
			update_damier(col, ligne, couleur_j);
			encadrement(col,ligne, couleur_j);
			getScoreAndCheckFinish();
			return 1;
		} 
	}
	return 0;
}

/* Fonction permettant changer nom joueur blanc dans cadre Score */
void set_label_J1(char *texte)
{
	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object (p_builder, "label_J1")), texte);
}

/* Fonction permettant de changer nom joueur noir dans cadre Score */
void set_label_J2(char *texte)
{
	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object (p_builder, "label_J2")), texte);
}

/* Fonction permettant de changer score joueur blanc dans cadre Score */
void set_score_J1(int score)
{
	char *s;
	
	s=malloc(5*sizeof(char));
	sprintf(s, "%d\0", score);
	
	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object (p_builder, "label_ScoreJ1")), s);
}

/* Fonction permettant de récupérer score joueur blanc dans cadre Score */
int get_score_J1(void)
{
	const gchar *c;
	
	c=gtk_label_get_text(GTK_LABEL(gtk_builder_get_object (p_builder, "label_ScoreJ1")));
	
	return atoi(c);
}

/* Fonction permettant de changer score joueur noir dans cadre Score */
void set_score_J2(int score)
{
	char *s;
	
	s=malloc(5*sizeof(char));
	sprintf(s, "%d\0", score);
	
	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object (p_builder, "label_ScoreJ2")), s);
}

/* Fonction permettant de récupérer score joueur noir dans cadre Score */
int get_score_J2(void)
{
	const gchar *c;
	
	c=gtk_label_get_text(GTK_LABEL(gtk_builder_get_object (p_builder, "label_ScoreJ2")));
	
	return atoi(c);
}


/* Fonction appelee lors du clique sur une case du damier */
static void coup_joueur(GtkWidget *p_case)
{
	int col, lig, type_msg, nb_piece, score;
	char buf[MAXDATASIZE];
	
	// Traduction coordonnees damier en indexes matrice damier
	coord_to_indexes(gtk_buildable_get_name(GTK_BUILDABLE(gtk_bin_get_child(GTK_BIN(p_case)))), &col, &lig);

	/***** TO DO *****/
	//tester si coup valide
	//Modifier damier et affichage
	//Envoyer l'information 
	//Quand on place une case
	if(jouer(col, lig, couleur) == 1){
		sprintf(buf, "%u, %u",  htons((uint16_t) col), htons((uint16_t) lig));
		send(newsockfd , buf , strlen(buf) , 0); 
		gele_damier();
	}
}

/* Fonction retournant texte du champs adresse du serveur de l'interface graphique */
char *lecture_addr_serveur(void)
{
	GtkWidget *entry_addr_srv;
	
	entry_addr_srv = (GtkWidget *) gtk_builder_get_object(p_builder, "entry_adr");
	
	return (char *)gtk_entry_get_text(GTK_ENTRY(entry_addr_srv));
}

/* Fonction retournant texte du champs port du serveur de l'interface graphique */
char *lecture_port_serveur(void)
{
	GtkWidget *entry_port_srv;
	
	entry_port_srv = (GtkWidget *) gtk_builder_get_object(p_builder, "entry_port");
	
	return (char *)gtk_entry_get_text(GTK_ENTRY(entry_port_srv));
}

/* Fonction retournant texte du champs login de l'interface graphique */
char *lecture_login(void)
{
	GtkWidget *entry_login;
	
	entry_login = (GtkWidget *) gtk_builder_get_object(p_builder, "entry_login");
	
	return (char *)gtk_entry_get_text(GTK_ENTRY(entry_login));
}

/* Fonction retournant texte du champs adresse du cadre Joueurs de l'interface graphique */
char *lecture_addr_adversaire(void)
{
	GtkWidget *entry_addr_j2;
	
	entry_addr_j2 = (GtkWidget *) gtk_builder_get_object(p_builder, "entry_addr_j2");
	
	return (char *)gtk_entry_get_text(GTK_ENTRY(entry_addr_j2));
}

/* Fonction retournant texte du champs port du cadre Joueurs de l'interface graphique */
char *lecture_port_adversaire(void)
{
	GtkWidget *entry_port_j2;
	
	entry_port_j2 = (GtkWidget *) gtk_builder_get_object(p_builder, "entry_port_j2");
	
	return (char *)gtk_entry_get_text(GTK_ENTRY(entry_port_j2));
}

/* Fonction affichant boite de dialogue si partie gagnee */
void affiche_fenetre_gagne(void)
{
	GtkWidget *dialog;
		
	GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
	
	dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_builder_get_object(p_builder, "window1")), flags, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "Fin de la partie.\n\n Vous avez gagné!!!", NULL);
	gtk_dialog_run(GTK_DIALOG (dialog));
	
	gtk_widget_destroy(dialog);
}

/* Fonction affichant boite de dialogue si partie perdue */
void affiche_fenetre_perdu(void)
{
	GtkWidget *dialog;
		
	GtkDialogFlags flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
	
	dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_builder_get_object(p_builder, "window1")), flags, GTK_MESSAGE_INFO, GTK_BUTTONS_CLOSE, "Fin de la partie.\n\n Vous avez perdu!", NULL);
	gtk_dialog_run(GTK_DIALOG (dialog));
	
	gtk_widget_destroy(dialog);
}

/* Fonction appelee lors du clique du bouton Se connecter */
static void clique_connect_serveur(GtkWidget *b)
{
	/***** TO DO *****/
	
}

/* Fonction desactivant bouton demarrer partie */
void disable_button_start(void)
{
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object (p_builder, "button_start"), FALSE);
}

/* Fonction traitement signal bouton Demarrer partie */
static void clique_connect_adversaire(GtkWidget *b)
{
	if(newsockfd==-1)
	{
		//Allow to clear the FD for the client first connection
		disableAccept = 1;

		//Set the client color to 0 -> Noir
		couleur = 0;

		//Init the damier
		init_interface_jeu();

		// Deactivation bouton demarrer partie
        gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object (p_builder, "button_start"), FALSE);
		
		// Recuperation  adresse et port adversaire au format chaines caracteres
		addr_j2=lecture_addr_adversaire();
		port_j2=lecture_port_adversaire();
		
		printf("[Port joueur : %d] Adresse j2 lue : %s\n",port, addr_j2);
		printf("[Port joueur : %d] Port j2 lu : %s\n", port, port_j2);

		
		pthread_kill(thr_id, SIGUSR1); 
	}
}

/* Fonction desactivant les cases du damier */
void gele_damier(void)
{
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA1"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB1"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC1"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD1"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE1"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF1"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG1"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH1"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA2"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB2"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC2"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD2"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE2"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF2"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG2"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH2"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA3"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB3"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC3"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD3"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE3"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF3"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG3"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH3"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA4"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB4"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC4"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD4"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE4"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF4"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG4"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH4"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA5"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB5"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC5"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD5"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE5"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF5"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG5"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH5"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA6"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB6"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC6"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD6"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE6"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF6"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG6"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH6"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA7"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB7"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC7"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD7"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE7"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF7"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG7"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH7"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA8"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB8"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC8"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD8"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE8"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF8"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG8"), FALSE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH8"), FALSE);
}

/* Fonction activant les cases du damier */
void degele_damier(void)
{
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA1"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB1"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC1"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD1"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE1"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF1"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG1"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH1"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA2"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB2"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC2"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD2"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE2"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF2"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG2"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH2"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA3"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB3"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC3"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD3"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE3"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF3"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG3"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH3"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA4"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB4"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC4"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD4"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE4"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF4"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG4"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH4"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA5"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB5"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC5"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD5"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE5"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF5"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG5"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH5"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA6"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB6"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC6"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD6"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE6"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF6"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG6"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH6"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA7"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB7"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC7"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD7"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE7"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF7"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG7"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH7"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxA8"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxB8"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxC8"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxD8"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxE8"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxF8"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxG8"), TRUE);
	gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object(p_builder, "eventboxH8"), TRUE);
}

/* Fonction permettant d'initialiser le plateau de jeu */
void init_interface_jeu(void)
{
	// Initilisation du damier (D4=blanc, E4=noir, D5=noir, E5=blanc)
	change_img_case(3, 3, 1);
	change_img_case(4, 3, 0);
	change_img_case(3, 4, 0);
	change_img_case(4, 4, 1);

	//Init matrice damier
	damier[3][3] = 1;
	damier[4][3] = 0;
	damier[3][4] = 0;
	damier[4][4] = 1;

	// Initialisation des scores et des joueurs
	if(couleur==1)
	{
		set_label_J1("Vous");
		set_label_J2("Adversaire");
	}
	else
	{
		set_label_J1("Adversaire");
		set_label_J2("Vous");
	}

	set_score_J1(2);
	set_score_J2(2);
	
	/***** TO DO *****/
	
}

/* Fonction reinitialisant la liste des joueurs sur l'interface graphique */
void reset_liste_joueurs(void)
{
	GtkTextIter start, end;
	
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(p_builder, "textview_joueurs")))), &start);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(p_builder, "textview_joueurs")))), &end);
	
	gtk_text_buffer_delete(GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(p_builder, "textview_joueurs")))), &start, &end);
}

/* Fonction permettant d'ajouter un joueur dans la liste des joueurs sur l'interface graphique */
void affich_joueur(char *login, char *adresse, char *port)
{
	const gchar *joueur;
	
	joueur=g_strconcat(login, " - ", adresse, " : ", port, "\n", NULL);
	
	gtk_text_buffer_insert_at_cursor(GTK_TEXT_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(gtk_builder_get_object(p_builder, "textview_joueurs")))), joueur, strlen(joueur));
}

static void connect_socket_adversaire(int sig)
{
		printf("Connexion adversaire [Reception signal SIGUSR1] sig=%d\n", sig);
		
		/* Cas où de l'envoie du signal par l'interface graphique pour connexion au joueur adverse */

		// Create a new socket 
		// sur adresse ip & port renseigné
		struct sockaddr_in addrinfo;
				
		addrinfo.sin_family = AF_INET;
		addrinfo.sin_port = htons(atoi(port_j2));

		if(inet_pton(AF_INET, addr_j2 , &(addrinfo.sin_addr))<=0)  
		{ 
			perror("\nInvalid address\n"); 
			exit(1);
		}

		if((newsockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			perror("client: socket");
			exit(1);
		}

		//Close sockfd
		close(sockfd);
		FD_CLR(sockfd, &master);

		FD_SET(newsockfd, &master);
		if(newsockfd>fdmax)
		{
			fdmax=newsockfd;
		}

		if(connect(newsockfd, (struct sockaddr*)&addrinfo, sizeof(addrinfo)) == -1)
		{ 
			close(newsockfd);
			perror("client: connect");
			exit(1);
		}else {
			printf("Client connected \n");
		}

}

/* Fonction exécutée par le thread gérant les communications à travers la socket */
static void * f_com_socket(void *p_arg)
{
	int i, nbytes, col, lig;
	
	char buf[MAXDATASIZE], *tmp, *p_parse;
	int len, bytes_sent, t_msg_recu;

	sigset_t signal_mask_org, signal_mask;
	struct sigaction signal_action;
//   int fd_signal;
	
	uint16_t type_msg, col_j2;
	uint16_t ucol, ulig;
	
	/* Association signal SIGUSR1 fonction callback connect_socket_adversaire() */
	
	memset (&signal_action, 0, sizeof(signal_action));
	signal_action.sa_handler = connect_socket_adversaire;
	
	if(sigaction(SIGUSR1, &signal_action, 0))
	{
			perror ("pb sigaction");
			return 1;      
	}
	
	sigemptyset(&signal_mask);
	sigaddset(&signal_mask, SIGUSR1);
		
//   if(pthread_sigmask(SIG_BLOCK, &signal_mask, NULL) == -1)
	if(pthread_sigmask(SIG_BLOCK, &signal_mask, &signal_mask_org) == -1)
	{
		printf("[Pourt joueur %d] Erreur sigprocmask\n", port);
		
		return 0;
	}


	while(1)
	{
		read_fds=master;	// copie des ensembles
		
		if((pselect(fdmax+1, &read_fds, &write_fds, NULL, NULL, &signal_mask_org)==-1) && errno != EINTR)
		{
			perror("Problème avec select");
			exit(4);
		}

		if(disableAccept == 1) {
			FD_CLR(sockfd, &read_fds);
		}
		
		printf("[Port joueur %d] Entree dans boucle for\n", port);
		for(i=0; i<=fdmax; i++)
		{
			printf("[Port joueur %d] newsockfd=%d, iteration %d boucle for\n", port, newsockfd, i);

			if(FD_ISSET(i, &read_fds))
			{
				if(i==sockfd)
				{ // Acceptation connexion adversaire
		
					printf("New player connection\r\n");

					struct sockaddr client = {0};
					socklen_t sinsize;
					newsockfd = accept(sockfd, (struct sockaddr *)&client, &sinsize);
					if (newsockfd == -1)
					{
						printf("Error accept() %d", newsockfd);
					}
					else
					{
						FD_SET(newsockfd, &master);
						if(newsockfd>fdmax)
						{
							fdmax=newsockfd;
						}
						
						//Set the server color to 1 -> Blanc
						couleur = 1;

						//Init the damier
						init_interface_jeu();
						gele_damier();

						printf("Server connected\n");
					}

					gtk_widget_set_sensitive((GtkWidget *) gtk_builder_get_object (p_builder, "button_start"), FALSE);
				} else { 
					
					char buffer[MAXDATASIZE] = {0};
					char *token, *saveptr;
					uint16_t uCol, uLigne;
					int col, ligne;

					//Read the message
					read( newsockfd , buffer, 1024); 

					//Parse the message
					token=strtok_r(buffer, ",", &saveptr);
					sscanf(token, "%hu", &uCol);
					token=strtok_r(NULL, ",", &saveptr);
					sscanf(token, "%hu", &uLigne);
					col = (int) ntohs(uCol);
					ligne = (int) ntohs(uLigne);

					jouer(col, ligne, couleur ^ 1 );

					degele_damier();
				
				}
			}
			
		}
		
	}
	
	return NULL;
}


int main (int argc, char ** argv)
{
	 int i, j, ret;

	 if(argc!=2)
	 {
		 printf("\nPrototype : ./othello num_port\n\n");
		 
		 exit(1);
	 }
	 
	 /* Initialisation graphique multi-threads */
	 XInitThreads();
	 
	 /* Initialisation de GTK+ */
	 gtk_init (& argc, & argv);
	 
	 /* Creation d'un nouveau GtkBuilder */
	 p_builder = gtk_builder_new();
 
	 if (p_builder != NULL)
	 {
			/* Chargement du XML dans p_builder */
			gtk_builder_add_from_file (p_builder, "UI_Glade/Othello.glade", & p_err);
 
			if (p_err == NULL)
			{
				 /* Recuparation d'un pointeur sur la fenetre. */
				 GtkWidget * p_win = (GtkWidget *) gtk_builder_get_object (p_builder, "window1");

 
				 /* Gestion evenement clic pour chacune des cases du damier */
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA1"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB1"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC1"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD1"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE1"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF1"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG1"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH1"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA2"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB2"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC2"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD2"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE2"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF2"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG2"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH2"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA3"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB3"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC3"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD3"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE3"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF3"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG3"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH3"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA4"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB4"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC4"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD4"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE4"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF4"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG4"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH4"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA5"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB5"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC5"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD5"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE5"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF5"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG5"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH5"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA6"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB6"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC6"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD6"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE6"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF6"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG6"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH6"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA7"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB7"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC7"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD7"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE7"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF7"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG7"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH7"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxA8"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxB8"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxC8"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxD8"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxE8"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxF8"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxG8"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "eventboxH8"), "button_press_event", G_CALLBACK(coup_joueur), NULL);
				 
				 /* Gestion clic boutons interface */
				 g_signal_connect(gtk_builder_get_object(p_builder, "button_connect"), "clicked", G_CALLBACK(clique_connect_serveur), NULL);
				 g_signal_connect(gtk_builder_get_object(p_builder, "button_start"), "clicked", G_CALLBACK(clique_connect_adversaire), NULL);
				 
				 /* Gestion clic bouton fermeture fenetre */
				 g_signal_connect_swapped(G_OBJECT(p_win), "destroy", G_CALLBACK(gtk_main_quit), NULL);
				 
				 
				 
				 /* Recuperation numero port donne en parametre */
				 port=atoi(argv[1]);
					
				 /* Initialisation du damier de jeu */
				 for(i=0; i<8; i++)
				 {
					 for(j=0; j<8; j++)
					 {
						 damier[i][j]=-1; 
					 }  
				 }

		 
				 /***** TO DO *****/
				 
				 // Initialisation socket et autres objets, et création thread pour communications avec joueur adverse
			 
				struct sockaddr_in addrinfo;
				
				addrinfo.sin_family = AF_INET;
				addrinfo.sin_port = htons(port);
				addrinfo.sin_addr.s_addr = INADDR_ANY;

				if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
					perror("server: socket");
					return 1;
				}
				if(bind(sockfd, (struct sockaddr*)&addrinfo, sizeof(addrinfo)) == -1)
				{ 
					close(sockfd);
					perror("server: bind");
					return 1;
				}

				listen(sockfd, 5);

				FD_SET(sockfd, &master);
				if(sockfd>fdmax)
				{
					fdmax=sockfd;
				}

				 /** Création du thread **/
				 if((ret = pthread_create(&thr_id, NULL, f_com_socket, NULL)) == -1) {
					 perror("error creating thread");
				 } 

				 gtk_widget_show_all(p_win);
				 gtk_main();

			}
			else
			{
				 /* Affichage du message d'erreur de GTK+ */
				 g_error ("%s", p_err->message);
				 g_error_free (p_err);
			}
	 }
 
 
	 return EXIT_SUCCESS;
}
