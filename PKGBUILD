# Maintainer: jt <joacotom1721@gmail.com>
pkgname=zenv
pkgver=0.1.0
pkgrel=1
pkgdesc="Personal shell environment toolkit: persistent vars, modes, cwd recovery"
arch=('x86_64')
url="https://github.com/joako1721/zenv"
license=('MIT')
depends=()
makedepends=('gcc')
source=("$pkgname-$pkgver.tar.gz::$url/archive/refs/tags/v$pkgver.tar.gz")
sha256sums=('SKIP')  # replace with `makepkg -g` output before publishing

build() {
	cd "$pkgname-$pkgver"
	make PREFIX=/usr
}

check() {
	cd "$pkgname-$pkgver"
	make test
}

package() {
	cd "$pkgname-$pkgver"
	make DESTDIR="$pkgdir" PREFIX=/usr install
}
