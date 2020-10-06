/*****************************************************************************/
/*  In this header file, we include the declarations provided in the         */
/*  fcgi-spec.html file. Whenver possible, we use these declarations as is.  */
/*****************************************************************************/

#define FCGI_LISTENSOCK_FILENO 0

typedef struct {
    unsigned char version;
    unsigned char type;
    unsigned char requestIdB1;
    unsigned char requestIdB0;
    unsigned char contentLengthB1;
    unsigned char contentLengthB0;
    unsigned char paddingLength;
    unsigned char reserved;
} FCGI_Header;

typedef struct {
    unsigned char roleB1;
    unsigned char roleB0;
    unsigned char flags;
    unsigned char reserved[5];
} FCGI_BeginRequestBody;

typedef struct {
    unsigned char appStatusB3;
    unsigned char appStatusB2;
    unsigned char appStatusB1;
    unsigned char appStatusB0;
    unsigned char protocolStatus;
    unsigned char reserved[3];
} FCGI_EndRequestBody;


#define FCGI_MAX_CONTENT_LEN ((1 << 16) - 1 ) // 65535
#define FCGI_MAX_PADDING_LEN (256 - 1 )

#define FCGI_HEADER_LEN        sizeof(FCGI_Header)
#define FCGI_BEGIN_REQUEST_LEN sizeof(FCGI_BeginRequestBody)
#define FCGI_END_REQUEST_LEN   sizeof(FCGI_EndRequestBody)


// FCGI_Header version
#define FCGI_VERSION_1           1

// FCGI_Header type
#define FCGI_BEGIN_REQUEST       1
#define FCGI_ABORT_REQUEST       2
#define FCGI_END_REQUEST         3
#define FCGI_PARAMS              4
#define FCGI_STDIN               5
#define FCGI_STDOUT              6
#define FCGI_STDERR              7
#define FCGI_DATA                8
#define FCGI_GET_VALUES          9
#define FCGI_GET_VALUES_RESULT  10
#define FCGI_UNKNOWN_TYPE       11
#define FCGI_MAXTYPE (FCGI_UNKNOWN_TYPE)

// FCGI_Header requestId
#define FCGI_NULL_REQUEST_ID     0


// FCGI roles
#define FCGI_RESPONDER      1
#define FCGI_AUTHORIZER     2
#define FCGI_FILTER         3

// FCGI protocolStatus
#define FCGI_REQUEST_COMPLETE 0
#define FCGI_CANT_MPX_CONN    1
#define FCGI_OVERLOADED       2
#define FCGI_UNKNOWN_ROLE     3


typedef struct {
  unsigned char nameLengthB0;  /* nameLengthB0  >> 7 == 0 */
  unsigned char valueLengthB0; /* valueLengthB0 >> 7 == 0 */
  // unsigned char nameData[nameLength];
  // unsigned char valueData[valueLength];
} FCGI_NameValuePair11;

typedef struct {
  unsigned char nameLengthB0;  /* nameLengthB0  >> 7 == 0 */
  unsigned char valueLengthB3; /* valueLengthB3 >> 7 == 1 */
  unsigned char valueLengthB2;
  unsigned char valueLengthB1;
  unsigned char valueLengthB0;
  // unsigned char nameData[nameLength];
  // unsigned char valueData[valueLength ((B3 & 0x7f) << 24) + (B2 << 16) + (B1 << 8) + B0];
} FCGI_NameValuePair14;

typedef struct {
  unsigned char nameLengthB3;  /* nameLengthB3  >> 7 == 1 */
  unsigned char nameLengthB2;
  unsigned char nameLengthB1;
  unsigned char nameLengthB0;
  unsigned char valueLengthB0; /* valueLengthB0 >> 7 == 0 */
  // unsigned char nameData[nameLength ((B3 & 0x7f) << 24) + (B2 << 16) + (B1 << 8) + B0];
  // unsigned char valueData[valueLength];
} FCGI_NameValuePair41;

typedef struct {
  unsigned char nameLengthB3;  /* nameLengthB3  >> 7 == 1 */
  unsigned char nameLengthB2;
  unsigned char nameLengthB1;
  unsigned char nameLengthB0;
  unsigned char valueLengthB3; /* valueLengthB3 >> 7 == 1 */
  unsigned char valueLengthB2;
  unsigned char valueLengthB1;
  unsigned char valueLengthB0;
  // unsigned char nameData[nameLength ((B3 & 0x7f) << 24) + (B2 << 16) + (B1 << 8) + B0];
  // unsigned char valueData[valueLength ((B3 & 0x7f) << 24) + (B2 << 16) + (B1 << 8) + B0];
} FCGI_NameValuePair44;
