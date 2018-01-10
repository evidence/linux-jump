JUMP-Linux
==========

Introduction
------------

JUMP-linux is a Software Distributed Shared Memory (SDSM) for Linux.

It is based on JUMP [1] which, in turn, is based on JIAJIA 1.1.
According to [1] JUMP has better performance of JIAJIA either version 1.1 and
2.1.

This version has been modified to run only on modern Linux systems, supporting
i386, x86_64, ARM and ARM64 architectures. The porting activity has been done
in the context of the AXIOM EU project [2].

The copyright is specified by the COPYING file.

Build
-----

To build type:

	make

Configuration
-------------

Edit ```~/.jiahosts``` and add the list of enabled hosts:

       <hostname> <username> XXX

The machine itself must be the first entry.

Add the enabled hosts to ```~/.rhosts``` as well:

       <hostname> <username>

Usage
-----

Some examples of usage are available in the ```apps/``` directory.

Run
---

The application binary must be put and started from <username>'s home
directory.

To enable automatic login between the machines:

        mkdir .ssh
        cd .ssh
        ssh-keygen -t rsa
        cat id_rsa.pub | ssh <user>@<machine> 'cat >> .ssh/authorized_keys'

See [3] for further information.


REFERENCES
----------

* [1] The JUMP Software DSM System: http://www.snrg.cs.hku.hk/srg/html/jump.htm
* [2] The AXIOM EU project: http://www.axiom-project.eu
* [3] SSH login without password: http://www.linuxproblem.org/art_9.html
