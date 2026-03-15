.PHONY: picsimlab nano upload

picsimlab:
	arduino-cli compile --build-property compiler.cpp.extra_flags="-DPICSIMLAB" --warnings more

nano:
	arduino-cli compile --profile nano
	arduino-cli upload --profile nano

uno:
	arduino-cli compile --profile uno
	arduino-cli upload --profile uno

upload:
	arduino-cli upload
