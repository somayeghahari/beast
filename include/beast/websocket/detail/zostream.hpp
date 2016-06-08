//
// Copyright (c) 2013-2016 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BEAST_WEBSOCKET_DETAIL_ZOSTREAM_HPP
#define BEAST_WEBSOCKET_DETAIL_ZOSTREAM_HPP

#include <beast/core/buffer_concepts.hpp>
#include <beast/core/error.hpp>
#include <boost/asio/buffer.hpp>

#include <beast/core/impl/zlib/zlib.h>

namespace beast {
namespace websocket {
namespace detail {

class zostream
{
    bool inited_ = false;
    z_stream zs_;

public:
    zostream()
    {
        zs_.zalloc = Z_NULL;
        zs_.zfree = Z_NULL;
        zs_.opaque = Z_NULL;
    }

    ~zostream()
    {
        clear();
    }

    void
    clear()
    {
        if(inited_)
        {
            inited_ = false;
            deflateEnd(&zs_);
        }
    }

    void
    init()
    {
        clear();
        zs_.avail_in = 0;
        zs_.next_in = Z_NULL;
        auto const result = deflateInit2(&zs_,
            Z_DEFAULT_COMPRESSION,
            Z_DEFLATED, -15,
            4, // memory level 1-9
            Z_DEFAULT_STRATEGY
        );
        inited_ = true;
    }

    template<class DynamicBuffer, class ConstBufferSequence>
#if GENERATING_DOCS
    std::size_t
#else
    typename std::enable_if<
        ! std::is_convertible<ConstBufferSequence,
            boost::asio::const_buffer>::value,
                std::size_t>::type
#endif
    write(DynamicBuffer& dynabuf,
        ConstBufferSequence const& buffers, bool fin, error_code& ec)
    {
        static_assert(beast::is_DynamicBuffer<DynamicBuffer>::value,
            "DynamicBuffer requirements not met");
        static_assert(beast::is_ConstBufferSequence<ConstBufferSequence>::value,
            "ConstBufferSequence requirements not met");
        using boost::asio::buffer_size;
        std::size_t n = 0;
        for(auto it = buffers.begin(); it != buffers.end();)
        {
            auto cur = it++;
            n += write(dynabuf, *cur,
                fin && it == buffers.end(), ec);
            if(ec)
                break;
        }
        return n;
    }

    template<class DynamicBuffer>
    std::size_t
    write(DynamicBuffer& dynabuf,
        boost::asio::const_buffer in, bool fin, error_code& ec)
    {
        static_assert(beast::is_DynamicBuffer<DynamicBuffer>::value,
            "DynamicBuffer requirements not met");
        using boost::asio::buffer_cast;
        using boost::asio::buffer_size;
        std::size_t n = 0;
        zs_.avail_in = buffer_size(in);
        zs_.next_in = const_cast<Bytef*>(
            buffer_cast<Bytef const*>(in));
        std::size_t prev = 0;
        do
        {
            dynabuf.commit(prev);
            static std::size_t constexpr amount = 16384;
            std::size_t tot = 0;
            auto const dmb = dynabuf.prepare(fin ? 16384 :
                deflateBound(&zs_, zs_.avail_in));
            for(auto const& b : dmb)
            {
                zs_.avail_out = buffer_size(b);
                zs_.next_out = buffer_cast<Bytef*>(b);
                auto const result = deflate(&zs_,
                    fin ? Z_FULL_FLUSH : Z_NO_FLUSH);
                if( result != Z_OK)
                {
                    ec = boost::system::errc::make_error_code(
                        boost::system::errc::invalid_argument);
                    return 0;
                }
                n += buffer_size(b) - zs_.avail_out;
                tot += buffer_size(b) - zs_.avail_out;
                if(zs_.avail_out > 0)
                    break;
            }
            prev = tot;
        }
        while(zs_.avail_out == 0);
        if(fin)
        {
            assert(prev >= 4);
            prev -=4;
        }
        dynabuf.commit(prev);
        return n;
    }
};

} // detail
} // websocket
} // beast

#endif
