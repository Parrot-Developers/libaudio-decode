# libaudio-decode - Audio decoding library

_libaudio-decode_ is a C library to handle the decoding of audio on
various platforms with a common API.

## Implementations

The following implementations are available:

* Fraunhofer FDK AAC (software decoding)

The application can force using a specific implementation or let the library
decide according to what is supported by the platform.

## Dependencies

The library depends on the following Alchemy modules:

* libaudio-defs
* libfutils
* libmedia-buffers
* libmedia-buffers-memory
* libulog
* (optional) fdk-aac (for FDK AAC support)

## Building

Building is activated by enabling _libaudio-decode_ in the Alchemy build
configuration.

## Operation

Operations are asynchronous: the application pushes buffers to decode in the
input queue and is notified of decoded frames through a callback function.

Some decoders need the input buffers to be originating from its own buffer
pool; when the input buffer pool returned by the library is not _NULL_ it must
be used and input buffers cannot be shared with other audio pipeline elements.

### Threading model

The library is designed to run on a _libpomp_ event loop (_pomp_loop_, see
_libpomp_ documentation). All API functions must be called from the _pomp_loop_
thread. All callback functions (frame_output, flush or stop) are called from
the _pomp_loop_ thread.

## Testing

The library can be tested using the provided _adec_ command-line tool which
takes as input an AAC-encoded file and can optionally output a raw WAV file.

To build the tool, enable _adec_ in the Alchemy build configuration.

For a list of available options, run

    $ adec -h
