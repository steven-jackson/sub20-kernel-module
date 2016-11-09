obj-m := sub20.o
obj-m += sub20-uart.o

checkpatch=/usr/src/linux-headers-`uname -r`/scripts/checkpatch.pl

all:
	make -C /lib/modules/`uname -r`/build C=2 W=1 M=$(CURDIR) modules
	${checkpatch} --no-signoff --no-tree --emacs -f ./*.c
	${checkpatch} --no-signoff --no-tree --emacs -f ./*.h

install:
	make -C /lib/modules/`uname -r`/build M=$(CURDIR) modules_install

clean:
	make -C /lib/modules/`uname -r`/build M=$(CURDIR) clean
