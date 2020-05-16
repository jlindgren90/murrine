# Maintainer: John Lindgren <john@jlindgren.net>
# Maintainer: Sergej Pupykin <pupykin.s+arch@gmail.com>
# Contributor: Andrea Scarpino <andrea@archlinux.org>
# Contributor: Roman Kyrylych <Roman.Kyrylych@gmail.com>
# Contributor: detto <detto-brumm@freenet.de>
# Maintainer: Daniel J Griffiths <ghost1227@archlinux.us>

pkgname=gtk-engine-murrine
pkgver=0.98.2
pkgrel=3
pkgdesc="GTK2 engine to make your desktop look like a 'murrina', an italian word meaning the art glass works done by Venicians glass blowers."
arch=('x86_64')
url="http://cimitan.com/murrine/project/murrine"
license=('LGPL3')
depends=('gtk2')
makedepends=('intltool')

build() {
  cd ..
  ./autogen.sh \
    --prefix=/usr \
    --enable-animation \
    --enable-animationrtl
  sed -i -e 's/ -shared / -Wl,-O1,--as-needed\0/g' libtool
  make
}

package() {
  cd ..
  make DESTDIR="$pkgdir" install
}
