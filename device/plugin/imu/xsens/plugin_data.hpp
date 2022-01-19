HERA_PLUGIN_DATA_DEFINE_START(XsensData, 0x0203)

bool is_synced;
int64_t timestamp_intrinsic;
MessageHeader message;
// ....
// Followed by message data;

HERA_PLUGIN_DATA_DEFINE_END