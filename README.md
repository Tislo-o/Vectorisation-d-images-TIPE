Commandes pour compiler:
debug: gcc -g -fsanitize=address main.c -o main -lm
release(avec optimisations): gcc -DNDEBUG -O3 main.c -o main -lm

Exécuter:
./main nom_de_l_image_source nom_du_resultat_svg
