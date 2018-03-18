default : ngp
ngp : main.c main.h
		gcc main.c -o ngp
clean : rm ngp