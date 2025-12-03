fractal.sk = 241kb

##Parsing
Parsing the pgm file involved processing the header and the pixel information separately. 

The header was decomposed into its different pieces of information which was then stored in a struct.

The pixel information was processed by looping through the rest of the bytes individually.

To optimise the size of the output file, pixels along the same row that were of the same value were grouped together so that a single line operation could represent them.


##Producing sketch instructions
A combination of structs, arrays and functions were used to organise the various sequences of binary instructions into templates for common cases, such as changing colour, drawing, and changing coordinate locations. 

While parsing, these templates were filled with the present information from the pixels, and injected into the output file.
