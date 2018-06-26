makese: deposer.c retirer.c
	gcc -Wall -Wextra -Werror -g -o deposer deposer.c
	gcc -Wall -Wextra -Werror -g -o retirer retirer.c
	gcc -Wall -Wextra -Werror -g -o lister lister.c
	gcc -Wall -Wextra -Werror -g -o demon demon.c
		
	chmod ug+s deposer
	chmod ug+s retirer
	chmod ug+s lister
	chmod ug+s demon
	
	mkdir -m 777 /tmp/spool/
	chmod +x script2.sh
	
	./script2.sh .
