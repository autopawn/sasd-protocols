#include "packets.h"

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <sys/socket.h>

#define TRY(ret)   \
    if (ret == -1) \
        return -1;

int _test_packet_1_deserialize(buffer_t data, struct test_packet_1* payload)
{
    TRY(buffer_pop_int32(data, &payload->data1));
    TRY(buffer_pop_char(data, &payload->data2));
    return 0;
}

int _test_packet_2_deserialize(buffer_t data, struct test_packet_2* payload)
{
    TRY(buffer_pop_int32(data, &payload->data1));
    TRY(buffer_pop_int16(data, &payload->data2));
    return 0;
}

int _handshake_deserialize(buffer_t data, struct handshake* payload)
{
    TRY(buffer_pop_int32(data, &payload->client_id));
    return 0;
}

int _plain_text_deserialize(buffer_t data, struct plain_text* payload)
{
    TRY(buffer_pop_string(data, &payload->text, 256));
    return 0;
}

int packet_deserialize(buffer_t data, packet_t* packet, int parse_sender)
{
    TRY(buffer_pop_int16(data, &packet->size));

    if (parse_sender) {
        TRY(buffer_pop_int32(data, &packet->sender_id));
    }
    char ptype;
    TRY(buffer_pop_char(data, &ptype));
    packet->ptype = ptype;

    switch (ptype) {
    case TEST_PACKET_1:
        TRY(_test_packet_1_deserialize(data, &packet->payload.packet1));
        break;
    case TEST_PACKET_2:
        TRY(_test_packet_2_deserialize(data, &packet->payload.packet2));
        break;
    case HANDSHAKE:
        TRY(_handshake_deserialize(data, &packet->payload.handshake));
        break;
    case PLAIN_TEXT:
        TRY(_plain_text_deserialize(data, &packet->payload.plain_text));
        break;
    }
    return 0;
}

buffer_t _test_packet_1_serialize(const struct test_packet_1* payload)
{
    // size + type + int32 + char
    int16_t size = 2 + 1 + 4 + 1;
    buffer_t buffer = buffer_create(size);
    buffer_push_int16(buffer, size);
    buffer_push_char(buffer, TEST_PACKET_1);
    buffer_push_int32(buffer, payload->data1);
    buffer_push_char(buffer, payload->data2);
    return buffer;
}

buffer_t _test_packet_2_serialize(const struct test_packet_2* payload)
{
    // size + type + int32 + int16
    int16_t size = 2 + 1 + 4 + 2;
    buffer_t buffer = buffer_create(size);
    buffer_push_int16(buffer, size);
    buffer_push_char(buffer, TEST_PACKET_2);
    buffer_push_int32(buffer, payload->data1);
    buffer_push_int16(buffer, payload->data2);
    return buffer;
}

buffer_t _handshake_serialize(const struct handshake* payload)
{
    // size + type + int32
    int16_t size = 2 + 1 + 4;
    buffer_t buffer = buffer_create(size);
    buffer_push_int16(buffer, size);
    buffer_push_char(buffer, HANDSHAKE);
    buffer_push_int32(buffer, payload->client_id);
    return buffer;
}

buffer_t _plain_text_serialize(const struct plain_text* payload)
{
    // size + type + string_size + var
    int16_t size = 2 + 1 + 2 + strlen(payload->text);
    buffer_t buffer = buffer_create(size);
    buffer_push_int16(buffer, size);
    buffer_push_char(buffer, PLAIN_TEXT);
    buffer_push_string(buffer, payload->text);
    return buffer;
}

buffer_t packet_serialize(const void* payload,
    enum payload_type ptype)
{
    switch (ptype) {
    case TEST_PACKET_1:
        return _test_packet_1_serialize(payload);
    case TEST_PACKET_2:
        return _test_packet_2_serialize(payload);
    case HANDSHAKE:
        return _handshake_serialize(payload);
    case PLAIN_TEXT:
        return _plain_text_serialize(payload);
    default:
        return NULL;
    }
}

void send_packet(int socket, const void* payload, enum payload_type ptype)
{
    auto_buffer_t data = packet_serialize(payload, ptype);
    send(socket, buffer_data(data), buffer_size(data), 0);
}

void print_packet(const packet_t* packet)
{
    switch (packet->ptype) {
    case NONE:
        printf("NONE\n");
        break;
    case TEST_PACKET_1:
        printf("test_packet_1 {\n");
        printf("\tsize = %d\n", packet->size);
        printf("\tsender_id = %d\n", packet->sender_id);
        printf("\tdata1 = %d\n", packet->payload.packet1.data1);
        printf("\tdata2 = %c\n}\n", packet->payload.packet1.data2);
        break;
    case TEST_PACKET_2:
        printf("test_packet_2 {\n");
        printf("\tsize = %d\n", packet->size);
        printf("\tsender_id = %d\n", packet->sender_id);
        printf("\tdata1 = %d\n", packet->payload.packet2.data1);
        printf("\tdata2 = %d\n}\n", packet->payload.packet2.data2);
        break;
    case HANDSHAKE:
        printf("handshake {\n");
        printf("\tsize = %d\n", packet->size);
        printf("\tsender_id = %d\n", packet->sender_id);
        printf("\tclient_id = %d\n}\n", packet->payload.handshake.client_id);
        break;
    case PLAIN_TEXT:
        printf("plain_text {\n");
        printf("\tsize = %d\n", packet->size);
        printf("\tsender_id = %d\n", packet->sender_id);
        printf("\ttext = %s\n}\n", packet->payload.plain_text.text);
        break;
    default:
        printf("UNKNOWN\n");
    }
}