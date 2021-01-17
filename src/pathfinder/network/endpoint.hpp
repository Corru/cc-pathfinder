#pragma once

#include <functional>
#include <string>
#include <set>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace network
{
    namespace beast = boost::beast;
    namespace http = beast::http;

    using request = http::request<http::dynamic_body>;
    using response = http::response<http::dynamic_body>;

    struct endpoint
    {
        using callback_t = std::function<void(const request&, response&)>;

        std::string path;
        http::verb  method;
        callback_t  callback;

        /// Returns ordering between two endpoints following by path and method
        friend auto operator<(const endpoint& left, const endpoint& right) -> bool
        {
            auto const res = left.path.compare(right.path);
            if (res < 0)
            {
                return true;
            }
            if (res == 0)
            {
                return left.method < right.method;
            }
            return false;
        }

        friend auto operator==(const endpoint& left, const endpoint& right) -> bool
        {
            return left.path == right.path and left.method == right.method;
        }
    };

    struct endpoints_collection : std::set<endpoint>
    {
        auto update(endpoint const& endpoint) & -> endpoints_collection&
        {
            this->insert(endpoint);
            return *this;
        }

        auto update(endpoint const& endpoint) && -> endpoints_collection&&
        {
            this->update(endpoint);
            return std::move(*this);
        }

        auto update(endpoint&& endpoint) & -> endpoints_collection&
        {
            this->insert(std::move(endpoint));
            return *this;
        }

        auto update(endpoint&& endpoint) && -> endpoints_collection&&
        {
            this->update(std::move(endpoint));
            return std::move(*this);
        }
    };
}