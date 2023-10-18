# The AOS implementation
The kernel main function is setup to do a randomized test on the rbtree.
It randomly shuffles 15 integers -> inserts those integers into the rbtree ->
attempts to remove all the nodes in order of index in randomized array.
`main()` should print `"You should see this message!\n"` after the randomized
test, but fails because it crashes. Run via `make qemu-nox`.

The exact steps are visualized in `rbtree_aos.pdf`. From what we can see in
this case, the insertion seems to go right but deletion violates some properties
of the redblack tree, which results in a crash in a later removal.
`rbtree_linux.pdf` shows for reference how the linux implementation performs
insertions and then deletions for the same order of operations.

gdb steps to get into the offending `rb_remove()` call:
```
b main.c:75
c
c
c
c
c
c
c
c
step
```

# The Linux implementation
To solve the rbtree problem for our openLSD OS, we ripped the linux kernel's 
implementation for rbtree and slightly modified it to comply with OpenLSD's
interface for rbtree operations. You can switch to the `linux` branch and run
`make qemu-nox` to see that the randomized test does indeed work with this 
implementation.

# Testing suite
We have added a testing suite for rbtree, which we used to validate the linux
implementation after translating.
* `cd` into `rbtree_tests` and build with `make aos` or `make linux`,
  for aos and linux implementations respectively.
* Test with `./rbtree_test <test_index>` where `<test_index>` is an integer
  in the range [0, 6]. Each test will output `.png` images of each step, which
  is the method we used to create `rbtree_aos.pdf` and `rbtree_linux.pdf`.
* Use `make pdf` to turn the image sequence into a single pdf and `make clean` to
  cleanup all the files. The dependencies are `dot` and `convert` for the testing
  suite.
* Testing parameters can be adjusted in `test.c` by changing the constants
  defined at the top of the file.

# Questions
* please email me at: a.s.tuzcu@student.vu.nl
