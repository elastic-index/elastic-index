//
//

#ifndef BLINDI_STX_ENDIANESSKEYSERIALIZER_H
#define BLINDI_STX_ENDIANESSKEYSERIALIZER_H

#include <arpa/inet.h>

/**
 * Currently hard-coded to work with long, but can be upgraded to be more generic.
 */
class SimpleEndianessKeySerializer {

public:
    void serialize(const void *key, void *serialized_key_output) {
        unsigned long key_val = *(reinterpret_cast<const unsigned long *>(key));

        *(reinterpret_cast<unsigned long *>(serialized_key_output)) = htonll(key_val);
    }

};


#endif //BLINDI_STX_ENDIANESSKEYSERIALIZER_H
