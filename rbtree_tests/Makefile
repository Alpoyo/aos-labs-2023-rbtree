linux:
	gcc rbtree.c test.c -O0 -g3 -o rbtree_test -w

aos:
	gcc rbtree.c test.c -O0 -g3 -DUSE_AOS -o rbtree_test -w

pdf:
	convert *.png rbtree_steps.pdf

clean:
	rm -f *.png *.pdf tmp rbtree_test