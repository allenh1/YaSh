YaSh: [![Build Status](https://travis-ci.org/allenh1/YaSh.svg?branch=master)](https://travis-ci.org/allenh1/YaSh)
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
Installation has two methods, supporting older systems (and strongly encouraging
newer systems).

Old:
-----
```
 $ ./autogen.sh
 $ ./configure --prefix=<prefix>
 $ make
 $ sudo make install
```

New:
-----
```
 $ mkdir build
 $ cd build
 $ cmake .. -DCMAKE_PREFIX=<prefix>
 $ make
 $ sudo make install
```

If you are installing it in your home directory (you know, if you're at school
or something), then you might do the following instead.

Old:
-----
```
 $ ./autogen.sh
 $ echo "export PATH=~/bin:${PATH}" >> ~/.bashrc
 $ ./configure --prefix=${HOME}
 $ make
 $ make install
```

New:
-----
If you are at Purdue (and CMake is _still_ less than 3.8.2), then just use the
old. Otherwise, please PR to complete this section!

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
 * ctrl + left arrow  to go to the previous space
 * ctrl + delete to remove the word to the right of the cursor
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

OSX Installation:
==================
OS X is, more or less, working. Follow the install guide above and be patient.
There will be more patches to come!

Windows Installation (By Harrison Chen @mechaHarry):
====================================================
Initially, Bash on Windows does not have the proper set of applications pre-installed, 
as evident of:
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
Therefore, the missing applications to install are as follows:
`autoconf`, `libtool`, `flex', and `bison`.
The command to install them is:

```
 $ sudo apt-get install build-essential autoconf libtool flex bison
```

When it prompts for 'y or n', press 'y' and 'enter' to proceed with the installation.
Once installation is complete, proceed to the next step by running:
```
 $ ./configure --prefix=/usr
```
When 'configure' is complete, there is a specific 'Makefile' edit 
to make the 'make' command work on Windows's Bash. 
'cd' into 'src' and use your preferred terminal-text-editor,
`pico`, `nano`, `vim`, or `emacs` to open the file named 'Makefile.am'
```
 $ cd src/
```
```
 $ emacs Makefile.am
```
In here, locate the lines containing '-std=c++14'
```
...
AM_CFLAGS = -g -O0 -lpthread -std=c++14
AM_CXXFLAGS = -g -O0 -lpthread -std=c++14
...
AM_CXXFLAGS = -O2 -fPIC -lpthread -std=c++14
...
```
and edit them to be '-std=c++11' like so:
```
...
AM_CFLAGS = -g -O0 -lpthread -std=c++11
AM_CXXFLAGS = -g -O0 -lpthread -std=c++11
...
AM_CXXFLAGS = -O2 -fPIC -lpthread -std=c++11
...
```
Once this has been done, save and exit the text editor, then run make by:
```
 $ make
```
Your installation should be complete and the final step is to install and run YaSh
```
 $ sudo make install
 $ yash
```
Welcome to YaSh on Bash on Ubuntu on Windows, otherwise known as YoBoUoW, pronounced yeahbwoi.

