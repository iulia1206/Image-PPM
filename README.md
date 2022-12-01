I found this homework to be very time consuming and extremely difficult
to understand. 

I created 2 additional structs: PPMPixel (for the colors) and PPM (stored
format of the file).
I created 2 pairs of functions to read and write from a binary file, to read
and write a compressed file and to read and write a PPM file.

Task1:
I read the PPM file using the specific function. Then, I put it in a
QuadtreeNode array and computed the colors of every block as the mean of 
pixels inside it. I then compressed every node based on the factor
compression formula. After compression, I took the resulting tree and wrote
the PPM file using the writeCompressedPPM function.

Task2:
It was the opposite process of Task1. I had to read the compressed file and
transform it in a PPM again. So, I used the readCompressedPPM function, and 
I reverted the compressed array back into a PPM structure. In the end, I
wrote the resulting PPM structure in the actual file.

Task3:
I used functions from both Task1 and Task2. I read the PPM file, compressed
the image into a QuadtreeNode array, decompress it, mirrored it accordingly 
(horizontal or vertical) and put the resulting array back into a PPM and 
from there to the actual mirrored image. 

 
