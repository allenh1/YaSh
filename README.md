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



