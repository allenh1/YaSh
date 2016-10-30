YaSh: [![build status](http://allen-software.com/gitlab/allensoftware/YaSh/badges/master/build.svg)](http://allen-software.com/gitlab/allensoftware/YaSh/commits/master)
====
YaSh is yet another Sh clone. It's my personal favorite, for obvious reasons (when it's working, that is).

Also, please note: if you are on the master branch, you are NOT on the stable
branch of this shell! If you are using this shell and are NOT a developer,
please checkout the current-stable branch.

```
 $ git checkout current-stable
```

After you are on the current-stable branch, proceed with the installation
as below.

Installation:
=============
Installation is as you would expect (well, for a source installation).

```
 $ ./autogen.sh
 $ ./configure --prefix=<prefix>
 $ make
 $ sudo make install
```

If you are installing it in your home directory (you know, if you're at school
or something), then you might do the following instead.

```
 $ ./autogen.sh
 $ echo "export PATH=~/bin:${PATH}" >> ~/.bashrc
 $ ./configure --prefix=${HOME}
 $ make
 $ make install
```

Uninstall:
==========
If you don't like the shell, you can remove it pretty easily.

```
 $ sudo make uninstall
```

Usage:
======
To run the installed shell, just type "yash" in your terminal.

```
 $ yash
```

Line Editing:
=============
In YaSh, one can...

 * ctrl + a to go to the beginning of the line
 * ctrl + e to go to the end of a line
 * tab to do tab completion
 * ctrl + c to send a kill signal to a process
 * ctrl + d to delete a character (backspace and delete keys also work)
 * ctrl + right arrow to go to the next space
 * ctrl + left arrow  to go to the previous space. 
 * use arrow keys to move through history

History:
========
 * You can do the bang-bang: !! will run the prior command.
 * !-<n> will run the think you ran n commands ago.

The only other thing to note (atm) is that you can use setenv.

In particular, if you want to change the way the terminal looks, you change
the PROMPT variable.

So, if you want to be boring, you can do the following.

```
 $ setenv PROMPT "YaSh => "
```
Windows Installation:
=====================
Initially, Bash on Windows (which I shall refer to as BoW from hereon out)
does not have the proper set of applications pre-installed, as evident of:
```
 $ ./autogen.sh
```
resulting in:
```
./autogen.sh: 4: ./autogen.sh: libtoolize: not found
./autogen.sh: 5: ./autogen.sh: aclocal: not found
./autogen.sh: 6: ./autogen.sh: automake: not found
./autogen.sh: 7: ./autogen.sh: autoreconf: not found
./autogen.sh: 8: ./autogen.sh: autoconf: not found
```
Therefore, the proper applications to install include:
`autoconf`, `libtool`, `flex, and `bison`.

```
 $ sudo apt-get install build-essential autoconf libtool flex bison
```

When it prompts for 'y or n' key in 'y' press return-key to proceed with the installation.
Once installation is complete, proceed to the next step by entering:
```
 $ ./configure --prefix=/usr
```
followed by
```
 $ make -j9
```
where the '-j9' is optional and make will work without it.

At this point, you should notice a slew of errors and text outputs containing, specifically:
```
g++: error: unrecognized command line option -std=c++14
make[3]: *** [main.o] Error 1
make[3]: *** Waiting for unfinished jobs.....g++: error: unrecognized command line option -std=c++14
make[3]: *** [command.o] Error 1
g++: error: unrecognized command line option -std=c++14
g++: error: unrecognized command line option -std=c++14
```
Do not fret, for this is an easy fix.
Either 'cd' into 'src' and use your preferred terminal-text-editor [pico, nano, vim, or emacs]
to open the file named 'Makefile.am'
```
 $ cd src/
```
```
 $ emacs Makefile.am
```
In here, find the lines containing '-std=c++11'
```
if DEBUG
AM_CFLAGS = -g -O0 -lpthread -std=c++11
AM_CXXFLAGS = -g -O0 -lpthread -std=c++11
else
AM_CFLAGS =  -O2 -fPIC -lpthread
AM_CXXFLAGS = -O2 -fPIC -lpthread -std=c++11
endif
```
and edit them to be '-std=c++14' like so:
```
...
AM_CFLAGS = -g -O0 -lpthread -std=c++14
AM_CXXFLAGS = -g -O0 -lpthread -std=c++14
...
AM_CXXFLAGS = -O2 -fPIC -lpthread -std=c++14
...
```
Once this has been done, save and exit the text editor and rerun make by:
```
 $ make -j9
```
Your installation should be complete and the final step is to install and run YaSh
```
 $ sudo make install
 $ yash
```
Welcome to YaSh on Bash on Ubuntu on Windows, otherwise known as YoBoUoW, pronounced yeahbwoi.