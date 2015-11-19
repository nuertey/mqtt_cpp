// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <iomanip>
#include <map>

#include <mqtt_client_cpp.hpp>

int main() {
    boost::asio::io_service ios;

    std::uint16_t pid_sub1;
    std::uint16_t pid_sub2;

    int count = 0;
    // Create no TLS client
    auto c = mqtt::make_client(ios, "test.mosquitto.org", 1883);

    // Setup client
    c.set_client_id("cid1");
    c.set_clean_session(true);

    // Setup handlers
    c.set_connack_handler(
        [&c, &pid_sub1, &pid_sub2]
        (bool sp, std::uint8_t connack_return_code){
            std::cout << "Connack handler called" << std::endl;
            std::cout << "Clean Session: " << std::boolalpha << sp << std::endl;
            std::cout << "Connack Return Code: "
                      << mqtt::connect_return_code_to_str(connack_return_code) << std::endl;
            if (connack_return_code == mqtt::connect_return_code::accepted) {
                pid_sub1 = c.subscribe("mqtt_client_cpp/topic1", mqtt::qos::at_most_once);
                pid_sub2 = c.subscribe("mqtt_client_cpp/topic2_1", mqtt::qos::at_least_once,
                                       "mqtt_client_cpp/topic2_2", mqtt::qos::exactly_once);
            }
        });
    c.set_close_handler(
        []
        (){
            std::cout << "closed." << std::endl;
        });
    c.set_error_handler(
        []
        (boost::system::error_code const& ec){
            std::cout << "error: " << ec.message() << std::endl;
        });
    c.set_puback_handler(
        [&c]
        (std::uint16_t packet_id){
            std::cout << "puback received. packet_id: " << packet_id << std::endl;
        });
    c.set_pubrec_handler(
        [&c]
        (std::uint16_t packet_id){
            std::cout << "pubrec received. packet_id: " << packet_id << std::endl;
        });
    c.set_pubcomp_handler(
        [&c]
        (std::uint16_t packet_id){
            std::cout << "pubcomp received. packet_id: " << packet_id << std::endl;
        });
    bool first = true;
    c.set_suback_handler(
        [&c, &first, &pid_sub1, &pid_sub2]
        (std::uint16_t packet_id, std::vector<boost::optional<std::uint8_t>> results){
            std::cout << "suback received. packet_id: " << packet_id << std::endl;
            for (auto const& e : results) {
                if (e) {
                    std::cout << "subscribe success: " << mqtt::qos::to_str(*e) << std::endl;
                }
                else {
                    std::cout << "subscribe failed" << std::endl;
                }
            }
            if (packet_id == pid_sub1) {
                c.publish_at_most_once("mqtt_client_cpp/topic1", "test1");
            }
            else if (packet_id == pid_sub2) {
                c.publish_at_least_once("mqtt_client_cpp/topic2_1", "test2_1");
                c.publish_exactly_once("mqtt_client_cpp/topic2_2", "test2_2");
            }
        });
    c.set_publish_handler(
        [&c, &count]
        (std::uint8_t header,
         boost::optional<std::uint16_t> packet_id,
         std::string topic_name,
         std::string contents){
            std::cout << "publish received. "
                      << "dup: " << std::boolalpha << mqtt::publish::is_dup(header)
                      << " pos: " << mqtt::qos::to_str(mqtt::publish::get_qos(header))
                      << " retain: " << mqtt::publish::is_retain(header) << std::endl;
            if (packet_id)
                std::cout << "packet_id: " << *packet_id << std::endl;
            std::cout << "topic_name: " << topic_name << std::endl;
            std::cout << "contents: " << contents << std::endl;
            if (++count == 3) c.disconnect();
        });

    // Connect
    c.connect();

    ios.run();
}
