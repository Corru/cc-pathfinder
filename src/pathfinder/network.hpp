#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <skyr/url.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

namespace network {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace net = boost::asio;
    using tcp = boost::asio::ip::tcp;
    using request = http::request<http::dynamic_body>;
    using response = http::response<http::dynamic_body>;

    struct endpoint {
        std::string path;
        http::verb  method;

        std::function<void(const request&, response&)> callback;

        friend auto operator<(const endpoint& left, const endpoint& right) -> bool {
            if (left.path == right.path) {
                return left.method < right.method;
            }
            return left.path < right.path;
        }

        friend auto operator==(const endpoint& left, const endpoint& right) -> bool {
            return left.path == right.path and left.method == right.method;
        }
    };

    class http_connection : public std::enable_shared_from_this<http_connection> {
    public:
        http_connection(tcp::socket socket)
            : socket_(std::move(socket))
        {}

        /// Initiate the asynchronous operations associated with the connection.
        template<typename ...Args, template<typename ...> class Container>
        auto start(Container<Args...> const& endpoints) -> void {
            read_request(endpoints);
            check_deadline();
        }

    private:
        /// The socket for the currently connected client.
        tcp::socket socket_;

        /// The buffer for performing reads.
        beast::flat_buffer buffer_{ 8192 };

        /// The request message.
        http::request<http::dynamic_body> request_;

        /// The response message.
        http::response<http::dynamic_body> response_;

        /// The timer for putting a deadline on connection processing.
        net::steady_timer deadline_{
            socket_.get_executor(),
            std::chrono::seconds(60)
        };

        /// Asynchronously receive a complete request message.
        template<typename ...Args, template<typename ...> class Container>
        auto read_request(Container<Args...> const& endpoints) -> void {
            auto self = shared_from_this();

            http::async_read(
                socket_,
                buffer_,
                request_,
                [self, &endpoints](beast::error_code ec, std::size_t bytes_transferred) {
                    boost::ignore_unused(bytes_transferred);
                    if (!ec) {
                        self->process_request(endpoints);
                    }
                });
        }

        /// Determine what needs to be done with the request message.
        template<typename ...Tail, template<typename ...> class Container>
        auto process_request(Container<endpoint, Tail...> const& endpoints) -> void {
            request_.target("http://service" + std::string{ request_.target() });
            response_.version(request_.version());
            response_.keep_alive(false);

            try {
                auto const url = skyr::url(std::string{ request_.target() });
                auto const dummy = endpoint{
                    .path = url.pathname(),
                    .method = request_.method(),
                };

                auto const it = endpoints.find(dummy);
                if (it != std::end(endpoints)) {
                    auto const& endpoint = *it;
                    std::invoke(endpoint.callback, request_, response_);
                }
                else {
                    // We return responses indicating an error if
                    // we do not recognize the request method.
                    response_.result(http::status::bad_request);
                    response_.set(http::field::content_type, "text/plain");
                    beast::ostream(response_.body())
                        << "404 "
                        << request_.method_string()
                        << " '"
                        << dummy.path
                        << "' not found";
                }
            }
            catch (std::exception const& e) {
                std::cerr << "error: " << e.what() << std::endl;

                response_.set(http::field::content_type, "text/plain");
                response_.body().clear();
                beast::ostream(response_.body())
                    << "error: "
                    << e.what();
            }

            write_response();
        }

        /// Asynchronously transmit the response message.
        auto write_response() -> void {
            auto self = shared_from_this();

            response_.content_length(response_.body().size());

            http::async_write(
                socket_,
                response_,
                [self](beast::error_code ec, std::size_t) {
                    self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                    self->deadline_.cancel();
                });
        }

        /// Check whether we have spent enough time on this connection.
        auto check_deadline() -> void {
            auto self = shared_from_this();

            deadline_.async_wait(
                [self](beast::error_code ec) {
                    if (!ec) {
                        // Close socket to cancel any outstanding operation.
                        self->socket_.close(ec);
                    }
                });
        }
    };
}