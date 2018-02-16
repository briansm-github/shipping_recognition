# Shipping Recognition
Experiments with 'recognizing' samples of the UK Shipping Forecast.

Here is some training/test data and code that attempts a simple 
type of speech recognition using the UK 'Shipping Forecast' as test data. 

There are lots of reasons for using the Shipping Forecast for experiments:

- The audio is alwasys studio quality, high SNR, low background noise
- There are only a hundred or so words/phrases to distinguish
- Multiple speakers, male and female
- effectively infinite available test data, new reports generated daily.

I'm uploading this data as 'fair use' as I personally doubt the BBC have
much interest in low-bitrate years-old weather forecast radio programmes.

Included here is a 1GB RAW audio file with about 120 reports taken from 2013,
downsampled to 8000 samples/sec (4000Hz) 16 bit RAW audio, 
making up about 20 hours of audio, to act as 'training' data, along with
30 reports from 2017 (5 hours of audio) as 'test' data.

chop.c is a small C program to extract a random 10 second
segment from the test data to run correlations on.

The idea then is to correlate segments of this sample against similar
sounding segments from the 'training' data.

features.c has code to generate feature data from either
the training or test samples. Given a RAW audio input it will generate
a binary file at 100fps of 9 integer ceptral vectors per frame based on
16 cubic-compressed Bark-scale frequency triangle filters, taken from a 512-bin
(15.56Hz resolution) FFT of 1.0 pre-emphasis input RAW audio.

Finally, compare.c has code that searches the training-data features
for the longest matching sections of audio that are within a certain
euchlidean distance of the test data. It then outputs these matching segments
as RAW audio samples to listen back to. A certain degree of dynamic time
warping is implementd to allow samples of 50%-200% length to potentially
correlate. 

Mostly I've been using this is a sandbox for trying different feature sets
to do the correlations on. This implementation - a hybrid of techniques from 
MFCC and PLP - seems to work best as far as I can tell, getting about 95%
accurate phrase matching.
The vectors can be further vector-quantized to around 14 bits
(16000 item codebook) while maintaining the correlation accuracy.

I'm also experimenting with
LPC-based features also though, which are computationally much simpler.

# Usage

Firstly, decompress the compressed training and test audio files.

The 'doit' shell script does an example run. 10 random seconds are picked
out from the test data and segments of sufficiently close audio from the
training sample are output as best matches. The resulting 2 audio files
can be converted to WAV format for comparison, or analysed via wavesurfer
or simiar audio programs. 

The 'compare' program uses the simplest approach - searching all 9 million 
training-data frames linearly - so will run very slow, but speed is not
the goal of this test-bed, more interest is in the accuracy of the results.

The 'example' directory givee an example of the audio files produced. 
It has a 'wav' subdirectory with conversions of the raw files. Resulting
segments usually end up being word-alligned, but not always. A more
heirachical serch would likely yeild better results.

A prequisite for compiling  would be the FFTW libraries. Just use 'make' to 
comile the 3 C files. Also Flac should be available to decompress the
audio files after download. Sox is useful for format coversions.
