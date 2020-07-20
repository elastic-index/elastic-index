
#ifndef BLINDI_STX_NAIVEINTSERIALIZER_H
#define BLINDI_STX_NAIVEINTSERIALIZER_H

#include <arpa/inet.h>
/**
 * naive serializer to use in tests.
 * copy of SimpleEndianessKeySerializer, with modification to int64
 */
class NaiveIntSerializer {
public:
    void serialize(const void *key, void *serialized_key_output) {
        unsigned int key_val = *(reinterpret_cast<const unsigned int *>(key));

        // hard-coded for int
        *(reinterpret_cast<unsigned int *>(serialized_key_output)) = htonl(key_val);
    }

};

#endif //BLINDI_STX_NAIVEINTSERIALIZER_H
