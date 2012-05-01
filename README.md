
# Influence field

Agents can connect to this program via libmapper, and specify their
position.
Their position is written to the field as a pixel, and then a GLSL
shader drives a convolution engine that propagates the "influence"
between pixels.
Agents can read from their position to observe agents that are
close-by, and decide how to move around the space.

# Dependencies

[LibMapper](http://libmapper.org)

    git co http://github.com/radarsat1/libmapper.git

Eventually this will make use of Qualia, a C++ engine for
reinforcement learning and other agent-type systems.

    git co http://github.com/sofian/qualia.git

# Compiling

Tested on Linux (Ubuntu) and Mac OS X.

    make

# Running

Run:

    ./influence

Then run any number of agents:

    ./agentConnect

These default agents just use forces based on the influence gradient.

# Affiliation

This project is developed as part of "e[m]erge", a collaboration
between [IDMIL][1], [XModal][2], [Moment Factory][3] and other
partners.

[1]: http://idmil.org
[2]: http://xmodal.hexagram.ca
[3]: http://www.momentfactory.com
