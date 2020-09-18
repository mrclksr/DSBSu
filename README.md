
# ABOUT

**DSBSu**
is a Qt frontend to
su(1).

# INSTALLATION

## Dependencies

**DSBSu**
depends on
*devel/qt5-buildtools*, *devel/qt5-core*, *devel/qt5-linguisttools*,
*devel/qt5-qmake*, *x11-toolkits/qt5-gui*,
and
*x11-toolkits/qt5-widgets*

## Getting the source code

	# git clone https://github.com/mrclksr/DSBSu.git

## Building and installation

	# cd DSBSu && qmake
	# make && make install

# USAGE

**dsbsu**
\[**-m** *message*]
\[**-u** *user*]
*command*  
**dsbsu**
**-a**
\[**-c** *command*]  
**dsbsudo**
\[*sudo options*]
*command*
\[*args ...*]

# DESCRIPTION

**dsbsu**
is a Qt front-end which allows executing commands as another user using
su(1).
**dsbsudo**
is a wrapper script for
sudo(8) which uses
**dsbsu**
for asking for a password if necessary.

# OPTIONS

**-a**

> Writes the entered password to stdout, and exits.

**-c**

> Shows the given
> *command*
> string in the password window.

**-m**

> Shows the given
> *message*
> in the password window.

**-u**

> Runs the
> *command*
> as the given
> *user*.
> If
> **-u**
> is not defined, root is assumed.

