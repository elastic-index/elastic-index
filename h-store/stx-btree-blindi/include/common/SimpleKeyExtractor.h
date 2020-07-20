//
//

#ifndef BLINDI_STX_SIMPLEKEYEXTRACTOR_H
#define BLINDI_STX_SIMPLEKEYEXTRACTOR_H


/**
 * Simply dereferences the pointer to extract the key.
 */
template <typename KeyType>
class SimpleKeyExtractor {

public:
    KeyType extractKey(char *tupleData) {
        return *(reinterpret_cast<KeyType *>(tupleData));
    }

};


#endif //BLINDI_STX_SIMPLEKEYEXTRACTOR_H
