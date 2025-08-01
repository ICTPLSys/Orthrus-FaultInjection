enum RetType {
    kError,
    kDeleted,
    kNotFound,
    kStored,
    kCreated,
    kEnd,
    kValue,
    kNumRetVals,
};

static const char *kRetVals[] = {"ERROR\r\n",  "DELETED\r\n", "NOT_FOUND\r\n",
                                 "STORED\r\n", "CREATED\r\n", "END\r\n",
                                 "VALUE "};
static const char kCrlf[] = "\r\n";

constexpr size_t KEY_LEN = 64;
constexpr size_t VAL_LEN = 256;