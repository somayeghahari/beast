//
// Copyright (c) 2013-2016 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BEAST_DETAIL_ZCODEC_HPP
#define BEAST_DETAIL_ZCODEC_HPP

#include <beast/core/error.hpp>
#include <boost/asio/buffer.hpp>
#include <algorithm>
#include <array>
#include <cstdint>

namespace beast {
namespace detail {

class z_istream
{
    enum state
    {
        s_block_begin,
        s_block_fin,
        s_block_type,

        s_plain,

        s_huff_fixed,

        s_huff_hlit,
        s_huff_hdist,
        s_huff_hclen,
        s_huff_lens,
        s_huff_codes
    };

    class bitstream
    {
        std::uint8_t n_ = 0;
        std::uint8_t v_ = 0;

        // reverse the bits
        static
        inline
        std::uint8_t
        reverse(std::uint8_t v)
        {
            return ((v * 0x0802LU & 0x22110LU) |
                    (v * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
        }

    public:
        template<
            std::size_t N, class Unsigned, class FwdIt>
        bool
        get(Unsigned& value, FwdIt& begin, FwdIt const& end)
        {
            static_assert(N > 0 && N <= 8, "");
            if(n_ < N)
            {
                if(begin == end)
                    return false;
                std::uint8_t const b = *begin++;
                value = reverse(
                    (v_ | (b << n_) & ((1 << N) - 1)) << (8 - N));
                n_ = 8 + n_ - N;
                v_ = b >> (8 - n_);
                return true;
            }
            value = reverse((v_ & ((1 << N) - 1)) << (8 - N));
            v_ >>= N;
            n_ -= N;
            return true;
        }

        template<class FwdIt>
        bool
        get1(bool& value, FwdIt& begin, FwdIt const& end)
        {
            if(n_ == 0)
            {
                if(begin == end)
                    return false;
                v_ = *begin++;
                value = v_ & 1;
                v_ >>= 1;
                n_ = 7;
                return true;
            }
            value = v_ & 1;
            v_ >>= 1;
            --n_;
            return true;
        }

        void
        flush()
        {
            n_ = 0;
            v_ = 0;
        }
    };

    bitstream bi_;
    std::uint16_t hlit_;
    std::uint16_t i_;
    state s_ = s_block_begin;
    std::uint8_t hdist_;
    std::uint8_t hclen_;
    std::uint8_t hlen_[19];
    std::unique_ptr<std::int16_t[]> hcode_;
    bool fin_;

    int hrv_;
    int hrl_;

public:
    template<class ConstBufferSequence>
    std::size_t
    write(ConstBufferSequence const& buffers, error_code& ec)
    {
        using boost::asio::buffer_size;
        std::size_t n = 0;
        for(auto it = buffers.begin(); it != buffers.end();)
        {
            auto cur = *it++;
            n += write_one(cur, ec);
            if(ec)
                return 0;
        }
        return n;
    }

    std::size_t
    write_one(boost::asio::const_buffer const& in, error_code& ec)
    {
        using boost::asio::buffer_cast;
        using boost::asio::buffer_size;
        auto const begin =  buffer_cast<std::uint8_t const*>(in);
        auto const end = begin + buffer_size(in);
        auto p = begin;
        auto const done =
            [&]
            {
                return p - buffer_cast<std::uint8_t const*>(in);
            };
        for(;;)
        {
            switch(s_)
            {
            case s_block_begin:
                s_ = s_block_fin;
                // fall through

            case s_block_fin:
            {
                std::uint8_t b;
                if(! bi_.get<1>(b, p, end))
                    return done();
                fin_ = b != 0;
                s_ = s_block_type;
                // fall through
            }

            case s_block_type:
            {
                std::uint8_t b;
                if(! bi_.get<2>(b, p, end))
                    return done();
                switch(b)
                {
                case 0:
                    s_ = s_plain;
                    break;

                case 1:
                    s_ = s_huff_fixed;
                    break;

                case 2:
                    // dynamic Huffman table
                    s_ = s_huff_hlit;
                    break;

                default:
                    ec = boost::system::errc::make_error_code(
                        boost::system::errc::invalid_argument);
                    return 0;
                }
                break;
            }
            
            case s_huff_hlit:
                if(! bi_.get<5>(hlit_, p, end))
                    return done();
                hlit_ += 257;
                s_ = s_huff_hdist;
                break;

            case s_huff_hdist:
                if(! bi_.get<5>(hdist_, p, end))
                    return done();
                hdist_ += 1;
                s_ = s_huff_hclen;
                break;

            case s_huff_hclen:
                if(! bi_.get<5>(hclen_, p, end))
                    return done();
                hclen_ += 4;
                i_ = 0;
                s_ = s_huff_lens;
                break;

            case s_huff_lens:
            {
                static std::array<std::uint8_t, 19> constexpr si = {{
                    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15}};
                if(! bi_.get<3>(hlen_[si[i_]], p, end))
                    return done();
                ++i_;
                if(i_ == si.size())
                {
                    i_ = 0;
                    hrv_ = -1;
                    hrl_ = 0;
                    hcode_.reset(new std::int16_t[hlit_ + hdist_]);
                    s_ = s_huff_codes;
                }
                break;
            }

            case s_huff_codes:
                if(hrl_ > 0)
                {
                    hcode_[i_++] = hrv_;
                    --hrl_;
                }
                else
                {
                    //std::int16_t sym = decode(
                }
                break;

            case s_huff_fixed:
                break;
            }
        }
    }
};

} // detail
} // beast

#endif
