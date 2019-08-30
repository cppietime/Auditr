all: audio/mus video/render

audio/mus:
	gcc -o audio/mus audio/mus.c audio/filter.c audio/fft.c audio/wav.c audio/vowel.c files/safeio.c

video/render:
	gcc -o video/render video/render.c video/pbm.c