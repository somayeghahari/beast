//
// Copyright (c) 2013-2016 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Test that header file is self-contained.
//#include <beast/http/basic_uri_parser.hpp>

#include <beast/unit_test/suite.hpp>

#include <beast/http/detail/rfc7230.hpp>
#include <beast/core/error.hpp>
#include <cstdint>

namespace beast {
namespace http {

enum parser_state : std::uint8_t
{
    s_dead = 0,

    s_uri_start,
    s_uri_path,
    s_uri_scheme,
    s_uri_scheme_slash,
    s_uri_scheme_slash2,
    s_uri_server_with_at,
    s_uri_server_start,
    s_uri_server,
    s_uri_query_start,
    s_uri_query,
    s_uri_frag_start,
    s_uri_frag
};

inline
bool
is_uichar(char c)
{
    /*
        userinfo      = *( unreserved / pct-encoded / sub-delims / ":" )
        unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
        pct-encoded   = "%" HEXDIG HEXDIG
        sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
                        "*" / "+" / "," / ";" / "="
    */
    static std::array<char, 256> constexpr tab = {{
        0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, // 0
        0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, // 16
        0, 1, 0, 0,  1, 0, 1, 1,  1, 1, 1, 1,  1, 1, 1, 0, // 32
        1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 0, 1,  0, 1, 0, 0, // 48
        0, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, // 64
        1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 0,  0, 0, 0, 1, // 80
        0, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, // 96
        1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 0,  0, 0, 1, 0, // 112
    }};
    return tab[static_cast<std::uint8_t>(c)] != 0;
}

inline bool is_urlchar(char c)
{
    return true;
}

#if 0
static const uint8_t normal_url_char[32] = {
/*   0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel  */
        0    |   0    |   0    |   0    |   0    |   0    |   0    |   0,
/*   8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si   */
        0    | T(2)   |   0    |   0    | T(16)  |   0    |   0    |   0,
/*  16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb */
        0    |   0    |   0    |   0    |   0    |   0    |   0    |   0,
/*  24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us  */
        0    |   0    |   0    |   0    |   0    |   0    |   0    |   0,
/*  32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '  */
        0    |   2    |   4    |   0    |   16   |   32   |   64   |  128,
/*  40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/*  48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/*  56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |   0,
/*  64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/*  72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/*  80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/*  88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/*  96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/* 104  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/* 112  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w  */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |  128,
/* 120  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del */
        1    |   2    |   4    |   8    |   16   |   32   |   64   |   0, };
#endif

/*

request-target  = origin-form / absolute-form / authority-form / asterisk-form

origin-form     = absolute-path [ "?" query ]
absolute-path   = 1*( "/" segment )
query           = [RFC3986], Section 3.4
segment         = *pchar
pchar           = unreserved / pct-encoded / sub-delims / ":" / "@"

absolute-form   = scheme ":" hier-part [ "?" query ]
scheme          = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )

authority-form  = [ userinfo "@" ] host [ ":" port ]
userinfo        = *( unreserved / pct-encoded / sub-delims / ":" )
host            = IP-literal / IPv4address / reg-name
port            = *DIGIT
IP-literal      = "[" ( IPv6address / IPvFuture  ) "]"
IPvFuture       = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
IPv6address     =                            6( h16 ":" ) ls32
                /                       "::" 5( h16 ":" ) ls32
                / [               h16 ] "::" 4( h16 ":" ) ls32
                / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
                / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
                / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
                / [ *4( h16 ":" ) h16 ] "::"              ls32
                / [ *5( h16 ":" ) h16 ] "::"              h16
                / [ *6( h16 ":" ) h16 ] "::"
h16             = 1*4HEXDIG
ls32            = ( h16 ":" h16 ) / IPv4address
IPv4address     = dec-octet "." dec-octet "." dec-octet "." dec-octet

asterisk-form   = "*"

unreserved      = ALPHA / DIGIT / "-" / "." / "_" / "~"
pct-encoded     = "%" HEXDIG HEXDIG
sub-delims      = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="

*/

inline
void
parse_uri_char(parser_state& s, char ch, error_code& ec)
{
    using beast::http::detail::is_alpha;
    auto const set =
        [&](parser_state s1)
        {
            s = s1;
            return;
        };
    switch(s)
    {
    case s_uri_start:
        if(ch == '/' || ch == '*')
            return set(s_uri_path);
        if(is_alpha(ch))
            return set(s_uri_scheme);
        break;

    case s_uri_scheme:
        if(is_alpha(ch))
            return;
        if(ch == ':')
            return set(s_uri_scheme_slash);
        break;

    case s_uri_scheme_slash:
        if(ch == '/')
            return set(s_uri_scheme_slash2);
        break;

    case s_uri_scheme_slash2:
        if(ch == '/')
            return set(s_uri_server_start);
        break;

    case s_uri_server_with_at:
        if(ch == '@')
            break;
        // fall through

    case s_uri_server_start:
        s = s_uri_server;
    case s_uri_server:
        if(ch == '/')
            return set(s_uri_path);
        if(ch == '?')
            return set(s_uri_query_start);
        if(ch == '@')
            return set(s_uri_server_with_at);
        if(is_uichar(ch) || ch == '[' || ch == ']')
            return;
        break;

    case s_uri_query_start:
        s = s_uri_query;
    case s_uri_query:
        if(is_urlchar(ch))
            return;
        if(ch == '?')
            return;
        if(ch == '#')
            return set(s_uri_frag_start);
        break;

    case s_uri_frag_start:
        if(is_urlchar(ch))
            return set(s_uri_frag);
        if(ch == '?')
            return set(s_uri_frag);
        if(ch == '#')
            return;
        break;

    case s_uri_frag:
        if(is_urlchar(ch))
            return;
        if(ch == '?' || ch =='#')
            return;
        break;
    }
    return set(s_dead);
}

class basic_uri_parser_test : public beast::unit_test::suite
{
public:
    template<class F>
    static
    void
    for_each_char(F const& f)
    {
        for(std::size_t i = 0; i < 256; ++i)
            f(static_cast<char>(i));
    }

    static
    bool
    test_uichar(char c)
    {
        switch(c)
        {
        case '-': case '.': case '_': case '~':
        case '!': case '$': case '&': case '\'': case '(': case ')':
        case '*': case '+': case ',': case ';':  case '=':
            return true;
        default:
            break;
        }
        if( (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9'))
            return true;
        return false;
    }

    void
    testTables()
    {
        for_each_char(
            [&](char c)
            {
                expect(test_uichar(c) == is_uichar(c));
            });
    }

    void
    testParser()
    {
        pass();
    }

    void run() override
    {
        testTables();
        testParser();
    }
};

BEAST_DEFINE_TESTSUITE(basic_uri_parser,http,beast);

} // http
} // beast
