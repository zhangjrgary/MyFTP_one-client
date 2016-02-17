struct message_s {
    char protocol[6]; /* protocol string (6 bytes) */
    char type; /* type (1 byte) */
    char status; /* status (1 byte) */
    int length; /* length (header + payload) (4 bytes) */
};
