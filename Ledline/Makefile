.PHONY: picsimlab nano upload

picsimlab:
	arduino-cli compile --build-property compiler.cpp.extra_flags="-DPICSIMLAB" --warnings more

nano:
	arduino-cli compile --profile nano

upload:
	arduino-cli upload
