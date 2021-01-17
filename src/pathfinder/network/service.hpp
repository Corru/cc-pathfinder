#pragma once

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <skyr/url.hpp>

#include "endpoint.hpp"

namespace network
{
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace net = boost::asio;
    using tcp = boost::asio::ip::tcp;

    class service : public std::enable_shared_from_this<service>
    {
    public:
        service(tcp::socket socket)
            : socket_(std::move(socket))
        {
        }

        // Initiate the asynchronous operations associated with the connection.
        auto start(endpoints_collection const& endpoints) -> void
        {
            // Asynchronously receive a complete request message.
            auto const read_request = [this, &endpoints]()
            {
                auto self = this->shared_from_this();

                http::async_read(
                    socket_,
                    buffer_,
                    request_,
                    [self, &endpoints](beast::error_code ec, std::size_t bytes_transferred)
                    {
                        boost::ignore_unused(bytes_transferred);
                        if (!ec)
                        {
                            self->process_request_to(endpoints);
                        }
                    });
            };

            read_request();
            check_deadline();
        }

    private:
        // The socket for the currently connected client.
        tcp::socket socket_;

        // The buffer for performing reads.
        beast::flat_buffer buffer_{ 8192 };

        // The request message.
        http::request<http::dynamic_body> request_;

        // The response message.
        http::response<http::dynamic_body> response_;

        // The timer for putting a deadline on connection processing.
        net::steady_timer deadline_{ socket_.get_executor(), std::chrono::seconds(60) };

        // Determine what needs to be done with the request message.
        auto process_request_to(endpoints_collection const& endpoints) -> void
        {
            response_.set(http::field::server, "Beast");
            response_.version(request_.version());
            response_.keep_alive(false);
            request_.target("http://service" + request_.target().to_string());

            auto const url   = skyr::url{ std::string{request_.target()} };
            auto const path  = url.pathname();
            auto const dummy = endpoint
            {
                .path   = path,
                .method = request_.method(),
            };

            auto const endpoint_ptr = std::find(std::begin(endpoints), std::end(endpoints), dummy);
            if (endpoint_ptr != std::end(endpoints))
            {
                try
                {
                    response_.result(http::status::ok);
                    std::invoke(endpoint_ptr->callback, request_, response_);
                }
                catch (std::exception const& e)
                {
                    std::cerr << "[error] " << e.what() << std::endl;
                    
                    response_.result(http::status::bad_request);
                    response_.set(http::field::content_type, "text/plain");
                    response_.body().clear();
                    beast::ostream(response_.body()) << "error: " << e.what();
                }
            }
            else
            {
                response_.result(http::status::not_found);
                response_.set(http::field::content_type, "text/plain");
                beast::ostream(response_.body()) << "endpoint not found";
            }

            write_response();
        }

        // Asynchronously transmit the response message.
        void write_response()
        {
            auto self = shared_from_this();

            response_.content_length(response_.body().size());

            http::async_write(
                socket_,
                response_,
                [self](beast::error_code ec, std::size_t)
                {
                    self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                    self->deadline_.cancel();
                }
            );
        }

        // Check whether we have spent enough time on this connection.
        void check_deadline()
        {
            auto self = shared_from_this();

            deadline_.async_wait(
                [self](beast::error_code ec)
                {
                    if (!ec)
                    {
                        // Close socket to cancel any outstanding operation.
                        self->socket_.close(ec);
                    }
                }
            );
        }
    };
}