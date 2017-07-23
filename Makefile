
all: tumblr-downloader prepare-gui build-gui

go-deps:
  $(shell go get github.com/dghubble/oauth1 && \
  go get github.com/elgs/gojq && \
  go get github.com/headzoo/surf)

tumblr-downloader:
	go build -o tumblr-downloader

prepare-gui:
	mv -fv tumblr-downloader tumblr-downloader-gui/

build-gui:
	$(MAKE) -C tumblr-downloader-gui/ -f Makefile.meta 

clean:
	$(MAKE) clean -f Makefile.meta -C tumblr-downloader-gui && \
	$(RM) tumblr-downloader-gui/tumblr-downloader tumblr-downloader-gui/Makefile

