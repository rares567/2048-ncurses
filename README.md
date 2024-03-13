Jocul este implementat intr-un singur window in ncurses, existand functii ce
dau clear la ecran, il recreaza si dau refresh (functie pt meniu, tabla joc,
sfarsit de joc). In jocul principal exista un select cu timeout 10ms care
ajuta la crearea a 2 "timeout-uri independente" (exista o variabila ce numara
cate timeout-uri au avut loc si calculeaza perioada de timp care a trecut). 
Cele 2 "timeout-uri" reprezinta unul de 1 sec pt a reafisa tabla (ca sa se
poata reafisa ora corecta) si unul ce poate fi setat prin variabile (define)
in incremente de 10ms. Tabelul de joc este creat prin trasarea unor linii si
adaugarea de caractere speciale la intersectii (caractere ACS). Exista o
variabila (define) prin care se poate schimba sansa in procente de aparitie
a nr 4 la adaugarea unei celule (in jocul original, sansa este de 10%, deci
90% pt a aparea nr 2). Algoritmul de miscare a tablei consta in verificarea
daca exista o celula cu 0 (care ar implica ca o miscare este posibila), 
shiftarea nr in directia respectiva si dupa verificarea daca se pot combina 
nr (care de asemenea ar constitui o miscare valida). Asemanator functioneaza
si functia de determinare a miscarilor valide (celule cu 0 sau combinatii).

Cerintele rezolvate sunt de la 1 la 5, bonus fiind salvarea high score-ului
modificarea usoara a timpului asteptat pt mutare automata si a sansei de
aparitia a nr 4, centrarea jocului  in functie de marimea ferestrei (sau
eroare daca nu e suficient de mare), posibilitatea de restartare sau iesire
dupa terminarea jocului.

Miscarea in meniu se face cu 'w'/sageata sus si 's'/sageata jos si selectarea
cu enter. In jocul propriu-zis, se misca cu wasd sau sagetile. Se revine in 
meniu prin tasta q, asa cum se si inchide tabla de joc dupa sfarsit. De
asemenea, se poate restarta jocul direct dupa sfarsit de joc prin tasta r.
