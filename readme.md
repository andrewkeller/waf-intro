Waf Intro
=========

This document serves as brief introduction to Waf, structured by examples.

Waf is an open source cross platform framework for writing build systems.  In particular, it provides:

* A concept of tasks
* Automatic task dependencies based on known file inputs and outputs
* Automatic task skipping when sources are demonstrably up-to-date
* Parallel execution by default

This introduction demonstrates the three major ways tasks are created: feature-based task generators, rule-based task
generators, and manually creating tasks.

This introduction is written for Waf 1.8.4.  To prevent this document from going out-of-date, the `waf` 1.8.4
executable is committed here.  If you would like to use the latest version of Waf, you can get it from
[GitHub](https://github.com/waf-project/waf).

For serious development using Waf, it's not a bad idea to read [the Waf Book](https://waf.io/book/) top to bottom at
least once, which provides an overview of what kinds of magic Waf is capable of, which is useful even if you don't
remember the details.  (_As a warning, the link to the Waf Book is not frozen at version 1.8.4; not a big deal, but
perhaps something to watch out for if something doesn't work._)

Building a web page
-------------------

As an example project, we're going to use Waf to compile a web page that contains a list of prime numbers.

A project built by Waf needs a top-level `wscript` file that tells Waf what to do.  This file is written in Python, and
contains an `option` method, a `configure` method, and a `build` method.  Let's start by creating the `wscript` file
now, with this contents:

    def options(ctx):
        print 'opt'
    
    def configure(ctx):
        print 'conf'
    
    def build(bld):
        print 'bld'

When building with Waf, you first configure the project, using `./waf configure`:

    $ ./waf configure
    Setting top to                           : /Waf Intro 
    Setting out to                           : /Waf Intro/build 
    opt
    conf
    'configure' finished successfully (0.003s)

Configuring the project runs both `options` (which provides an opportunity to declare command-line options) and
`configure` (which sets up the project, including creating the `build` directory).

Then, we build the project, using `./waf` (or `./waf build`):

    Waf: Entering directory `/Waf Intro/build'
    bld
    Waf: Leaving directory `/Waf Intro/build'
    'build' finished successfully (0.003s)

Next, we need a list of prime numbers, and we will show how task generators work to do it.  Waf has some built-in
functionality for compiling a fair number of languages.  We have a sample prime number generator in this folder written
in C++.  Let's use a task generator to compile that file into a working application.  First, we need to load Waf's C
support.  We do this in the `options` and `configure` methods:

    def options(ctx):
        ctx.load('compiler_cxx')
    
    def configure(ctx):
        ctx.load('compiler_cxx')

Next, because we changed what our `configure` method does, we must re-run `./waf configure`:

    $ ./waf configure
    Setting top to                           : /Waf Intro 
    Setting out to                           : /Waf Intro/build 
    Checking for 'clang++' (C++ compiler)    : /usr/bin/clang++ 
    'configure' finished successfully (0.155s)

Notice it checked for a C++ compiler, and then found one at `/usr/bin/clang++` (results may vary based on OS).  We are
now ready to build the example application that generates prime numbers.  To do this, we will use a task generator in
the `build` method:

    def build(bld):
        bld(features='cxx cxxprogram', source='primes.cpp', target='primes')

And now this is what happens when we build:

    $ ./waf
    Waf: Entering directory `/Waf Intro/build'
    [1/2] Compiling primes.cpp
    [2/2] Linking build/primes
    Waf: Leaving directory `/Waf Intro/build'
    'build' finished successfully (0.183s)

And, if we execute our `primes` app, we see it works:

    $ ./build/primes 7
    3
    5
    7
    11
    13
    17
    19

Next, we want to use these prime numbers to compile some HTML.  For this task, we will explore how custom tasks may be
created to do whatever we want.

First, at the top of our `wscript` file, we must import some of Waf's infrastructure:

    import re
    import subprocess
    from waflib import Task

Then, at the bottom of our `wscript` file, we will define a custom task that saves the list of primes to a file:

    class run_primes_task(Task.Task):
        color = 'PINK'
        def keyword(self):
            return 'Generating'
        def __str__(self):
            node = self.outputs[0]
        return node.path_from(node.ctx.launch_node())
        def run(self):
            p = subprocess.Popen([self.inputs[0].abspath(), '100'], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
            stdout, stderr = p.communicate()
            if stderr:
                Logs.warn(stderr)
            if p.returncode != 0:
                bld.fatal("primes tool failed with status %d" % (p.returncode,))
            self.outputs[0].parent.mkdir()
            self.outputs[0].write(stdout)

To use this task, we append this to our `build` method:

    t = run_primes_task(env = bld.env)
    t.set_inputs(bld.path.get_bld().make_node('primes'))
    t.set_outputs(bld.path.get_bld().make_node('primes.txt'))
    bld.add_to_group(t)

And this is what now happens when we run a build:

    $ ./waf
    Waf: Entering directory `/Waf Intro/build'
    [3/3] Generating build/primes.txt
    Waf: Leaving directory `/Waf Intro/build'
    'build' finished successfully (0.021s)

Waf knew to not rebuild the `primes` application, and ran the task to create the `primes.txt` file.

Next, we want to build our HTML file.  We'll use a custom task for this, too.  Again, at the bottom of the file, we
write a custom task:

    class our_magic_primes_html_task(Task.Task):
        color = 'GREEN'
        def keyword(self):
            return 'Generating'
        def __str__(self):
            node = self.outputs[0]
            return node.path_from(node.ctx.launch_node())
        def run(self):
            primes = self.inputs[0].read()
            primes = filter(None, re.split(r'[^0-9]+', primes))
            self.outputs[0].parent.mkdir()
            self.outputs[0].write('''<html><head><link rel="stylesheet" href="style.css"><title>Primes</title></head><body><h1>Primes!</h1><ul class="primes">'''
                                  + ''.join(['<li>%s</li>' % (prime,) for prime in primes]) + '''</ul></body></html>''')

And to execute it, we append this to our `build` method:

    t = our_magic_primes_html_task(env=bld.env)
    t.set_inputs(bld.path.get_bld().make_node('primes.txt'))
    t.set_outputs(bld.path.get_bld().make_node('webroot/primes.html'))
    bld.add_to_group(t)

And when we run `./waf`:

    $ ./waf
    Waf: Entering directory `/Waf Intro/build'
    [4/4] Generating build/webroot/primes.html
    Waf: Leaving directory `/Waf Intro/build'
    'build' finished successfully (0.012s)

Again, Waf knows not to run tasks that have already finished successfully.  If you edit the `primes.cpp` file and run
another build, you'll see Waf rebuild everything.

You can now see a file called `primes.html` has been added to the folder `build/webroot`.  You can open the file in a
web browser, and it displays!

And finally, what's life without a little style?  There's a CSS file in this directory.  Let's copy that to to next
to the generated HTML file, so it can be found by a browser.  Append this to the `build` method:

    bld(color='CYAN', rule='cp ${SRC} ${TGT}', source='style.css', target='webroot/style.css')

And again, rebuild:

    $ ./waf
    Waf: Entering directory `/Waf Intro/build'
    [5/5] Compiling style.css
    Waf: Leaving directory `/Waf Intro/build'
    'build' finished successfully (0.019s)

And in your web browser, if you refresh the file, you'll see the style has been updated.
