## Compilation
To compile noelle run `make noelle`
This will clone noelle if it doesn't already exist and build it.
If noelle does exist then this command will do nothing. If you made changes to noelle then `cd noelle ; make clean ; make uninstall ; make` to rebuild it.

To compile our passes run `make passes`
This will link the template run_me.sh script and build all passes with it.
If you only want to build one pass, `cd` to that directory and run `./run_me.sh`
