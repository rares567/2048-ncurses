#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <ctype.h>
#include <time.h>
#include <sys/select.h>

#define FOREVER 	1
#define KEYBOARD	0
#define MOVE_DELAY_S	5
#define MOVE_DELAY_10MS 0
#define CHANCE_OF_4	10 //% sanse

void to_string (char s[], int n);
int show_game (int maxrow, int maxcol, int a[][4], int *score);
void main_game (int maxrow, int maxcol, int a[][4], int *score);
void show_menu (int maxrow, int maxcol, int is_resume, int a[][4], int *score);
void init_array (int a[][4]);
int move_left (int a[][4]);
int move_right (int a[][4]);
int move_up (int a[][4]);
int move_down (int a[][4]);
int is_valid (int a[][4], int v[]);
void generate_cell (int a[][4]);
void game_over (int maxrow, int maxcol, int a[][4], int *score);
int has_2048 (int a[][4]);
int take_best_move (int a[][4], int *end);

int main() {
	srand(time(NULL));
	WINDOW *wnd = initscr();
	//Se va reține în nrows și ncols numărul maxim de linii și coloane
	int maxrow, maxcol;
	getmaxyx(wnd, maxrow, maxcol);
	//Se inhibă afișarea caracterelor introduse de la tastatură
	noecho();
	//Caracterele introduse sunt citite imediat - fără 'buffering'
	cbreak();
	//Se ascunde cursorul
	curs_set(0);
	//pentru interpretarea sagetilor fara a citi esc, [, A etc.
	keypad(stdscr, 1);
	int a[4][4], score = 0;
	show_menu(maxrow, maxcol, 0, a, &score);
	return 0;
}

//
void main_game (int maxrow, int maxcol, int a[][4], int *score) {
	fd_set read_descriptors;
	struct timeval timeout;
	FD_ZERO(&read_descriptors);
	FD_SET(KEYBOARD, &read_descriptors);
	timeout.tv_sec = 0;
	//10ms
	timeout.tv_usec = 10000;
	if (!show_game(maxrow, maxcol, a, score)) return;
	//end functioneaza pe logica negative (1 false, 0 true)
	int c, sel, v[4], end = 1, nr_timeouts = 0;
	while (FOREVER) {
		sel = select(1, &read_descriptors, NULL, NULL, &timeout);
		switch (sel) {
			case 1:
				c = getch();
				/*if (c == 91) {
					c = getchar();
				} else {
					c = tolower(c);
				}*/
				switch (tolower(c)) {
					case 'w':
					case KEY_UP:
						end = is_valid(a, v);
						/*la fiecare miscare se reseteaza nr de timeouturi
						  pt a se reseta numaratoarea*/
						nr_timeouts = 0;
						/*se reapeleaza is_valid() pt a verifica daca s-a
						  terminat jocul la aceasta miscare*/
						if (v[2]) {
							*score += move_up(a);
							end = is_valid(a, v);
						}
						if (!end || has_2048(a)) {
							game_over(maxrow, maxcol, a, score);
							endwin();
							return;
						}
					break;
					case 's':
					case KEY_DOWN:
						end = is_valid(a, v);
						nr_timeouts = 0;
						if (v[3]) {
							*score += move_down(a);
							end = is_valid(a, v);
						}
						if (!end || has_2048(a)) {
							game_over(maxrow, maxcol, a, score);
							endwin();
							return;
						}
					break;
					case 'a':
					case KEY_LEFT:
						end = is_valid(a, v);
						nr_timeouts = 0;
						if (v[0]) {
							*score += move_left(a);
							end = is_valid(a, v);
						}
						if (!end || has_2048(a)) {
							game_over(maxrow, maxcol, a, score);
							endwin();
							return;
						}
					break;
					case 'd':
					case KEY_RIGHT:
						end = is_valid(a, v);
						nr_timeouts = 0;
						if (v[1]) {
							*score += move_right(a);
							end = is_valid(a, v);
						}
						if (!end || has_2048(a)) {
							game_over(maxrow, maxcol, a, score);
							endwin();
							return;
						}
					break;
					case 'q':
						show_menu(maxrow, maxcol, 1, a, score);
					return;

				}
				show_game(maxrow, maxcol, a, score);
			break;
			case 0:
				nr_timeouts++;
				/*verificare multiplu de 100 <=> a trecut 1 sec de la
				  ultimul refresh (trebuie reafisat pt schimbare de ora)
				  Nu se reafiseaza la fiecare 10ms pt eficienta*/
				if (nr_timeouts % 100 == 0)
					show_game(maxrow, maxcol, a, score);
			break;
			case -1:
				printf("Select error");
			return;
		}
		//verificare cate timeout-uri au avut loc
		if (nr_timeouts == MOVE_DELAY_S * 100 + MOVE_DELAY_10MS) {
			*score += take_best_move(a, &end);
			//verificare daca s-a trminat jocul prin miscare automata
			if (!end || has_2048(a)) {
				game_over(maxrow, maxcol, a, score);
				endwin();
				return;
			}
			nr_timeouts = 0;
			show_game(maxrow, maxcol, a, score);
		}
		FD_SET(KEYBOARD, &read_descriptors);
		timeout.tv_sec = 0;
		timeout.tv_usec = 10000;
	}
}

//afiseaza meniul, cu sau fara optiunea de Resume
void show_menu (int maxrow, int maxcol, int is_resume, int a[][4], int *score) {
	/*
	Pentru a centra se va folosi diferenta dintre maxcol si lungimea sirului
	de caractere, impartit la 2. Centrarea pe verticala se face asemanator
	in functie de maxrow si inaltimea totala a meniului
	*/

	//clear pt a sterge tabelul cand se intra in meniu
	clear();
	int row_space, col_space, nr_rows;
	//pt centrarea randului cu New Game
	col_space = (maxcol - strlen("New Game")) / 2;
	if (is_resume) {
		nr_rows = 5;
		row_space = (maxrow - nr_rows) / 2;
		mvaddstr(row_space, col_space, "New Game");
		mvaddstr(row_space + 2, col_space, "Resume");
		mvaddstr(row_space + 4, col_space, "Quit");
	} else {
		nr_rows = 3;
		row_space = (maxrow - nr_rows) / 2;
		mvaddstr(row_space, col_space, "New Game");
		mvaddstr(row_space + 2, col_space, "Quit");
	}
	int row = row_space, col = col_space - 2;
	/* Se mută cursorul la poziția (row, col) */ 
	move(row, col);
	/* Se adaugă la poziția indicată de cursor caracterul '*' */
	addch('*');
	/* Se reflectă schimbările pe ecran */
	refresh();
	int c, new_row = row, new_col = col;
	/* Rămânem în while până când se primește tasta q */
	while (FOREVER) {
		c = getch();
		/* Daca se apasa una din sageti se va citi caracterul cu codul 91 '['
		   In cazul acesta, vom citi urmatorul caracter care va avea codul
		   65, 66 pt sageata sus, respectiv sageata jos
		
		if (c == 91) {
			c = getchar();
		} else {
			c = tolower(c);
		}*/
		/* Se determină noua poziție, în funcție de tasta apăsată
		 * Nu putem depași randurile meniului
		 * Enter are codul ascii 13
		 */
		switch (tolower(c)) {
			case 'w':
			case KEY_UP:
				if (row > row_space)
					new_row = row - 2;
				new_col = col;
				break;
			
			case 's':
			case KEY_DOWN:
				if (row + 1 < row_space + nr_rows)
					new_row = row + 2;
				new_col = col;
				break;
			
			//enter
			case 10:
				if (row == row_space) { //new game
					init_array(a);
					*score = 0;
					main_game(maxrow, maxcol, a, score);
					endwin();
					return;
				} else if (row == row_space + 2 && is_resume) { //resume
					main_game(maxrow, maxcol, a, score);
					endwin();
					return;
				} else { //quit
					endwin();
					return;
				}
		}

		/* Se șterge '*' din vechea poziție */
		move(row, col);
		addch(' ');
		
		/* Se adaugă '*' în noua poziție */
		move(new_row, new_col);
		addch('*');
		refresh();

		row = new_row;
		col = new_col;
	}
}

//afiseaza tabelul si nr din el sau eroare daca fereastra este prea mica
int show_game (int maxrow, int maxcol, int a[][4], int *score) {
	time_t t = time(NULL);
	struct tm time = *localtime(&t);
	int i, j, row_space, col_space, stats_col_space, stats_row_space;
	row_space = (maxrow - 17) / 2;
	col_space = (maxcol - 29) / 2;
	//pentru centrarea statisticilor
	stats_col_space = (col_space - 26) / 2;
	stats_row_space = (17 - 5) / 2 + row_space;
	clear();
	//verif daca se poate afisa tabelul
	if (maxrow < 17 || stats_col_space < 0) {
		endwin();
		printf("Consola este prea mica. Mariti fereastra si reincercati.\n");
		return 0;
	}
	//afisare high score
	FILE *f = fopen("data", "rb");
	if (f != NULL) {
		int high_score;
		//modificare pt a fi centrat cu high score
		stats_row_space = (17 - 7) / 2 + row_space;
		fread(&high_score, sizeof(int), 1, f);
		mvprintw(stats_row_space + 6, stats_col_space, "High score: %d", high_score);
		fclose(f);
	}
	//afisare data si ora
	mvprintw(stats_row_space, stats_col_space, "%d/%d/%d %02d:%02d:%02d",
	time.tm_mday, time.tm_mon + 1, time.tm_year + 1900, time.tm_hour, time.tm_min, time.tm_sec);
	//afisare scor
	mvprintw(stats_row_space + 2, stats_col_space, "Score: %d", *score);
	//afisare miscari valide
	mvprintw(stats_row_space + 4, stats_col_space, "Miscari valide: ");
	int valid_moves[4], dist;
	is_valid(a, valid_moves);
	dist = stats_col_space + strlen("Miscari valide: ");
	if (valid_moves[0]) {
		mvaddch(stats_row_space + 4, dist, ACS_LARROW);
		mvprintw(stats_row_space + 4, dist + 1, ", ");
		dist += 3;
	}
	if (valid_moves[1]) {
		mvaddch(stats_row_space + 4, dist, ACS_RARROW);
		mvprintw(stats_row_space + 4, dist + 1, ", ");
		dist += 3;
	}
	if (valid_moves[2]) {
		mvaddch(stats_row_space + 4, dist, ACS_UARROW);
		mvprintw(stats_row_space + 4, dist + 1, ", ");
		dist += 3;
	}
	if (valid_moves[3]) {
		mvaddch(stats_row_space + 4, dist, ACS_DARROW);
		mvprintw(stats_row_space + 4, dist + 1, ", ");
		dist += 3;
	}
	//suprascrie "," pusa in plus
	mvprintw(stats_row_space + 4, dist - 2, " ");
	//afisare tabel joc
	for (i = 0; i < 5; i++) {
		mvhline(row_space + (i * 4), col_space, 0, 29);
		mvvline(row_space, col_space + (i * 7), 0, 16);
	}
	char s[5];
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			//plus intersectii
			mvaddch(row_space + i * 4, col_space + j * 7, ACS_PLUS);
			to_string(s, a[i][j]);
			//pt nr de 2 cif si sub le plasam cu o col mai la dreapta pt centrare
			if (strcmp(s, "0") != 0) {
				if (strlen(s) < 3)
					mvprintw(row_space + i * 4 + 2, col_space + j * 7 + 3, "%s", s);
				else
					mvprintw(row_space + i * 4 + 2, col_space + j * 7 + 2, "%s", s);
			}
		}
		//T-uri intersectii
		mvaddch(row_space + i * 4, col_space, ACS_LTEE);
		mvaddch(row_space + i * 4, col_space + 28, ACS_RTEE);
		mvaddch(row_space, col_space + i * 7, ACS_TTEE);
		mvaddch(row_space + 16, col_space + i * 7, ACS_BTEE); 
	}
	//colturi
	mvaddch(row_space, col_space, ACS_ULCORNER);
	mvaddch(row_space, col_space + 28, ACS_URCORNER);
	mvaddch(row_space + 16, col_space, ACS_LLCORNER);
	mvaddch(row_space + 16, col_space + 28, ACS_LRCORNER);
	refresh();
	return 1;
}

//transforma din int in sir de caractere
void to_string (char s[], int n) {
	int k = 0, i;
	char aux[4]; //n este maxim 2048
	//obtine un sir de caractere inversat al nr original
	while (n) {
		aux[k++] = n % 10 + '0';
		n /= 10;
	}
	//inverseaza pt a fi in ordinea corecta
	for (i = 0; i < k; i++)
		s[k - i - 1] = aux[i];
	s[k] = '\0';
}

//initializeaza tabla de joc cu 2 casute
void init_array (int a[][4]) {
	//v este folosit in a determina daca s-a generat aceeasi pozitie consecutiv
	int i, j, v[4];
	for (i = 0 ; i < 4; i++)
		for (j = 0; j < 4; j++)
			a[i][j] = 0;
	//iteratiile sunt i = 0 si i = 2 pt ca vectorul sa fie format corect
	for (i = 0 ; i < 3; i += 2) {
		v[i] = rand() % 4;
		v[i + 1] = rand() % 4;
		/*rand() % x == 0 <=> 1/x probabilitate sa fie 0 (100/x % sanse)
		  cum sansele sunt date de CHANCE_OF_4 <=> CHANCE_OF_4 = 100/x
		  (in procente) deci x = 100/CHANCE_OF_4*/
		if (rand() % (100 / CHANCE_OF_4) == 0)
			a[v[i]][v[i + 1]] = 4;
		else
			a[v[i]][v[i + 1]] = 2;
	}
	/*daca cei 2 parametrii ai matricei sunt identici la ambele generari,
	  atunci s-a suprascris pozitia si este doar o casuta numerotata*/
	if (v[0] == v[2] && v[1] == v[3])
		init_array(a);
	/*int i, j, b[4][4] = {{2, 0, 2, 2},
		  				 {4, 0, 4, 4},
		  				 {8, 0, 8, 8},
		  				 {1024, 1024, 16, 16}};
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			a[i][j] = b[i][j];*/
}

//misca si combina elementele unei matrice la stanga
int move_left (int a[][4]) {
	int i, j, k, move_score = 0, nr_iterations;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 3; j++) {
			/*nr_iteratons pt a nu ramane blocat in while daca dupa
			  element sunt doar 0-uri (a[i][j] ar ramane 0 mereu)*/
			nr_iterations = 0;
			while (a[i][j] == 0 && nr_iterations < 4) {
				/*for-ul shifteaza elementele dar lasa ultimul element neschimbat
				  asfel, ultimul de pe linie trebuie reinitializat ca 0*/
				for (k = j; k < 3; k++)
					a[i][k] = a[i][k + 1];
				a[i][k] = 0;
				nr_iterations++;
			}
		}
		/*verif daca exista celule ce pot fi combinate
		  se verifica dupa for-ul anterior pt a fi mutate deja valorile
		  (daca exista celule ce se pot combina, vor fi adiacente)*/
		for (j = 0; j < 3; j++)
			if (a[i][j] == a[i][j + 1]) {
				a[i][j] <<= 1;
				//scorul castigat este valoarea noii casute create
				move_score += a[i][j];
				//dupa combinare si mai pot shifta restul elementelor 1 data
				for (k = j + 1; k < 3; k++)
					a[i][k] = a[i][k + 1];
				//apare un 0 mereu dupa o combinate de nr
				a[i][k] = 0;
			}
	}
	generate_cell(a);
	return move_score;
}

//misca si combina elementele unei matrice la dreapta
int move_right (int a[][4]) {
	//explicatiile sunt la fel ca la move_left()
	int i, j, k, move_score = 0, nr_iterations;
	for (i = 0; i < 4; i++) {
		for (j = 3; j >= 1; j--) {
			nr_iterations = 0;
			while (a[i][j] == 0 && nr_iterations < 4) {
				for (k = j; k >= 1; k--)
					a[i][k] = a[i][k - 1];
				a[i][k] = 0;
				nr_iterations++;
			}
		}
		for (j = 3; j >= 1; j--)
			if (a[i][j] == a[i][j - 1]) {
				a[i][j] <<= 1;
				move_score += a[i][j];
				for (k = j - 1; k >= 1; k--)
					a[i][k] = a[i][k - 1];
				a[i][k] = 0;
			}
	}
	generate_cell(a);
	return move_score;
}

//misca si combina elementele unei matrice in sus
int move_up (int a[][4]) {
	//explicatiile sunt la fel ca la move_left()
	int i, j, k, move_score = 0, nr_iterations;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 3; j++) {
			nr_iterations = 0;
			while (a[j][i] == 0 && nr_iterations < 4) {
				for (k = j; k < 3; k++)
					a[k][i] = a[k + 1][i];
				a[k][i] = 0;
				nr_iterations++;
			}
		}
		for (j = 0; j < 3; j++)
			if (a[j][i] == a[j + 1][i]) {
				a[j][i] <<= 1;
				move_score += a[j][i];
				for (k = j + 1; k < 3; k++)
					a[k][i] = a[k + 1][i];
				a[k][i] = 0;
			}
	}
	generate_cell(a);
	return move_score;
}

//misca si combina elementele unei matrice in sus
int move_down (int a[][4]) {
	//explicatiile sunt la fel ca la move_left()
	int i, j, k, move_score = 0, nr_iterations;
	for (i = 0; i < 4; i++) {
		for (j = 3; j >= 1; j--) {
			nr_iterations = 0;
			while (a[j][i] == 0 && nr_iterations < 4) {
				for (k = j; k >= 1; k--)
					a[k][i] = a[k - 1][i];
				a[k][i] = 0;
				nr_iterations++;
			}
		}
		for (j = 3; j >= 1; j--)
			if (a[j][i] == a[j - 1][i]) {
				a[j][i] <<= 1;
				move_score += a[j][i];
				for (k = j - 1; k >= 1; k--)
					a[k][i] = a[k - 1][i];
				a[k][i] = 0;
			}
	}
	generate_cell(a);
	return move_score;
}

/*modifica vectorul v[4] primit ca input in modul urmator:
  se adauga un 1 daca miscarea este valida pe o directie,
  fiecare pozitie din vector reprezinta o directie astfel:
  prima poz repr stanga, dupa dreapta, sus, jos
  (ex: 1011 inseamna ca miscari valide sunt stanga, sus si jos).
  De asemenea, returneaza 0, respectiv 1, daca nu exista miscari
  valide, respectiv exista macar o miscare valida (pentru cand
  nu este important CARE directie e buna, ci doar faptu ca exista
  miscari valide)*/
int is_valid (int a[][4], int v[]) {
	int i, j, k;
	//initializeaza vectorul ca fiind 0
	for (i = 0; i < 4; i++)
		v[i] = 0;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			/*Daca un element este 0 si mai exista macar un elem la
			  dreapta sa nenul insemna ca stanga este o mutare valida.
			  La fel pentru mutare valida la dreapta*/
			if (a[i][j] == 0) {
				//stanga
				for (k = j + 1; k < 4; k++)
					if (a[i][k]) {
						v[0] = 1;
						break;
					}
				//dreapta
				for (k = j - 1; k >= 0; k--)
					if (a[i][k]) {
						v[1] = 1;
						break;
					}
				//sus
				for (k = i + 1; k < 4; k++)
					if (a[k][j]) {
						v[2] = 1;
						break;
					}
				//jos
				for (k = i - 1; k >= 0; k--)
					if (a[k][j]) {
						v[3] = 1;
						break;
					}
			}
			/*Daca nu exista un 0 care sa repr o miscare valida,
			  este posibil ca sa fie 2 celule alaturate care sa
			  poata fi combinate, reprezentand o miscare valida.
			  Verificarea contorului e pt a elimina cazul in care
			  compara un element cu o val ce nu apartine vectorului*/
			else if (j < 3 && a[i][j] == a[i][j + 1]) {
				v[0] = 1;
				v[1] = 1;
			} else if (i < 3 && a[i][j] == a[i + 1][j]) {
				v[2] = 1;
				v[3] = 1;
			}
		}
	}
	//numara cate miscari valide exista (pentru return)
	for (i = 0; i < 4; i++)
		if (v[i])
			return 1;
	return 0;
}

//adauga o celula 2 sau 4 pe o pozitia aleatorie libera
void generate_cell (int a[][4]) {
	int b[16][2], i, j, k = 0, rand_poz;
	/*memoram pozitiile libere pentru ca la alegerea unei pozitii
	  aleatorii, sa fim siguri ca aceasta este si libera
	  (vom alege de fapt un element aleatoriu din b care va
	  corespunde unei pozitii aleatorii libere din a[][])*/
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			if (a[i][j] == 0) {
				b[k][0] = i;
				b[k++][1] = j;
			}
	//k reprezinta si nr de linii ale matricei b
	rand_poz = rand() % k;
	if (rand() % (100 / CHANCE_OF_4) == 0)
		a[b[rand_poz][0]][b[rand_poz][1]] = 4;
	else
		a[b[rand_poz][0]][b[rand_poz][1]] = 2;
}

//displays end of game screen
void game_over (int maxrow, int maxcol, int a[][4], int *score) {
	clear();
	/*int row_space = (maxrow - 3) / 2;
	mvprintw(row_space, (maxcol - 9) / 2, "Game Over");
	mvprintw(row_space + 2, (maxcol - 19) / 2, "Your score is: %d", score);*/
	//afisare tabel joc
	int i, j, row_space, col_space, msg_row_space;
	row_space = (maxrow - 17) / 2;
	col_space = (maxcol - 29) / 2;
	for (i = 0; i < 5; i++) {
		mvhline(row_space + (i * 4), col_space, 0, 29);
		mvvline(row_space, col_space + (i * 7), 0, 16);
	}
	char s[5];	
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			//plus intersectii
			mvaddch(row_space + i * 4, col_space + j * 7, ACS_PLUS);
			to_string(s, a[i][j]);
			//pt nr de 2 cif si sub le plasam cu o col mai la dreapta pt centrare
			if (strcmp(s, "0") != 0) {
				if (strlen(s) < 3)
					mvprintw(row_space + i * 4 + 2, col_space + j * 7 + 3, "%s", s);
				else
					mvprintw(row_space + i * 4 + 2, col_space + j * 7 + 2, "%s", s);
			}
		}
		//T-uri intersectii
		mvaddch(row_space + i * 4, col_space, ACS_LTEE);
		mvaddch(row_space + i * 4, col_space + 28, ACS_RTEE);
		mvaddch(row_space, col_space + i * 7, ACS_TTEE);
		mvaddch(row_space + 16, col_space + i * 7, ACS_BTEE); 
	}
	//colturi
	mvaddch(row_space, col_space, ACS_ULCORNER);
	mvaddch(row_space, col_space + 28, ACS_URCORNER);
	mvaddch(row_space + 16, col_space, ACS_LLCORNER);
	mvaddch(row_space + 16, col_space + 28, ACS_LRCORNER);
	//salvare (daca e cazul) si afisare high score
	FILE *f = fopen("data", "rb");
	msg_row_space = (17 - 7) / 2 + row_space;
	if (f != NULL) {
		int high_score;
		fread(&high_score, sizeof(int), 1, f);
		if (high_score < *score) {
			f = fopen("data", "wb");
			fwrite(score, sizeof(int), 1, f);
			//se recalculeaza pt a fi centrat cu mesajul nou afisat
			msg_row_space = (17 - 9) / 2 + row_space;
			mvprintw(msg_row_space + 8, (col_space - 15) / 2, "New high score!");
		}
	} else {
		//nu exista fisier
		msg_row_space = (17 - 9) / 2 + row_space;
		f = fopen("data", "wb");
		fwrite(score, sizeof(int), 1, f);
		mvprintw(msg_row_space + 8, (col_space - 15) / 2, "New high score!");
	}
	fclose(f);
	//afisare game over
	mvprintw(msg_row_space, (col_space - 9) / 2, "Game Over");
	mvprintw(msg_row_space + 2, (col_space - 19) / 2, "Your score is: %d", *score);
	mvprintw(msg_row_space + 4, (col_space - 15) / 2, "Press q to quit");
	mvprintw(msg_row_space + 6, (col_space - 16) / 2, "Press r to retry");
	refresh();
	char c;
	while (FOREVER) {
		c = getchar();
		switch (tolower(c)) {
			case 'q':
				return;
			case 'r':
				init_array(a);
				*score = 0;
				main_game(maxrow, maxcol, a, score);
				endwin();
				return;
		}
	}
}

//verifica daca exista celula cu 2048
int has_2048 (int a[][4]) {
	int i, j;
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			if (a[i][j] == 2048)
				return 1;
	return 0;
}

//verifica care miscare valida este cea mai buna si o executa
int take_best_move (int a[][4], int *end) {
	/*matricea score va memora scorul si fiecare directie in ordinea:
	  stanga (0), dreapta (1), sus (2), jos (3)*/
	int b[4][4], score[4][2], tmp, i, j, k = 0, v[4];
	is_valid(a, v);
	if (v[0]) {
		//se copiaza in b matricea a pt a nu modifica pe a
		for (i = 0; i < 4; i++)
			for (j = 0; j < 4; j++)
				b[i][j] = a[i][j];
		score[k][0] = move_left(b);
		score[k++][1] = 0;
	}
	if (v[1]) {
		for (i = 0; i < 4; i++)
			for (j = 0; j < 4; j++)
				b[i][j] = a[i][j];
		score[k][0] = move_right(b);
		score[k++][1] = 1;
	}
	if (v[2]) {
		for (i = 0; i < 4; i++)
			for (j = 0; j < 4; j++)
				b[i][j] = a[i][j];
		score[k][0] = move_up(b);
		score[k++][1] = 2;
	}
	if (v[3]) {
		for (i = 0; i < 4; i++)
			for (j = 0; j < 4; j++)
				b[i][j] = a[i][j];
		score[k][0] = move_down(b);
		score[k++][1] = 3;
	}
	/*ordoneaza descrescator scorurile, mentinand indexul
	  miscarii pt fiecare scor*/
	for (i = 0; i < k - 1; i++)
		for (j = i + 1; j < k; j++)
			if (score[i][0] < score[j][0]) {
				tmp = score[j][0];
				score[j][0] = score[i][0];
				score[i][0] = tmp;
				tmp = score[j][1];
				score[j][1] = score[i][1];
				score[i][1] = tmp;
			}	
	//primul rand contine miscarea cea mai buna
	switch (score[0][1]) {
		case 0:
			move_left(a);
		break;
		case 1:
			move_right(a);
		break;
		case 2:
			move_up(a);
		break;
		case 3:
			move_down(a);
	}
	//end functioneaza pe logica negativa
	if (v[0] == 0 && v[1] == 0 && v[2] == 0 && v[3] == 0)
		*end = 0;
	return score[0][0];
}