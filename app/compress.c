#include <zlib.h>

/// Compress a buffer using gzip.
/// Returns length of compressed buffer.
/// Compressed buffer is stored in `out`.
/// From <https://stackoverflow.com/a/57699371>.
int gzip(char* buf, int buflen, char* out, int outlen) {
    z_stream zs;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    zs.avail_in = (uInt)buflen;
    zs.next_in = (Bytef*)buf;
    zs.avail_out = (uInt)outlen;
    zs.next_out = (Bytef*)out;
    deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
    deflate(&zs, Z_FINISH);
    deflateEnd(&zs);
    return zs.total_out;
}
