paleofetch
==========

Like neofetch, but incomplete, written in ~500 lines of C instead of 10,000 lines of bash,
and runs a lot faster. Currently it only supports Arch Linux with X running. 

Example output:

![example output](example.png)

Compiling
---------

    make install

Usage
-----

    paleofetch

By default, `paleofetch` will cache certain  information (in `$XDG_CACHE_HOME/paleofetch`)
to speed up subsequent calls. To ignore the contents of the cache (and repopulate it), run

    paleofetch --recache

The cache file can safely be removed.

FAQ
---

Q: Do you really run neofetch every time you open a terminal?  
A: Yes, I like the way it looks and like that it causes my prompt to start midway
down the screen. I do acknowledge that the information it presents is not actually useful.
