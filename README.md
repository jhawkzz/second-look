# second-look
A PC desktop monitor program. I developed a really cool driver to prevent Windows End Task

What I love about this program is the driver I had to write to protect it. The nature of this app (PC monitoring) meant I couldn't allow a user 
to simply "End Task" on its executable. This lead me down a months long rabbit hole figuring out how to perform "kernel hooking", an undocumented
feature in 32bit Win NT platforms that allowed you to reroute kernel functions to other kernel-space functions by updating a function pointer table.
This, then, allowed me to write a driver that monitored a given process id and prevent its termination.

Better yet, just read this: https://github.com/jhawkzz/second-look/blob/main/CompiledArt/X9Suite/AppIntegrityDriver/Application%20Integrity.txt
