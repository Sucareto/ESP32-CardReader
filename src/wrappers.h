/*
库代码来自：https://github.com/spicetools/spicetools/tree/master/api/resources/arduino
删除了未使用的函数。
*/
#ifndef SPICEAPI_WRAPPERS_H
#define SPICEAPI_WRAPPERS_H

#define ARDUINOJSON_USE_LONG_LONG 1
#include "ArduinoJson.h"

#include <Arduino.h>
#include "connection.h"

// default buffer sizes
#ifndef SPICEAPI_WRAPPER_BUFFER_SIZE
#define SPICEAPI_WRAPPER_BUFFER_SIZE 256
#endif
#ifndef SPICEAPI_WRAPPER_BUFFER_SIZE_STR
#define SPICEAPI_WRAPPER_BUFFER_SIZE_STR 256
#endif

namespace spiceapi {

/*
      * Structs
      */

struct InfoAvs {
  String model, dest, spec, rev, ext;
};

// static storage
char JSON_BUFFER_STR[SPICEAPI_WRAPPER_BUFFER_SIZE_STR];

/*
      * Helpers
      */

uint64_t msg_gen_id() {
  static uint64_t id_global = 0;
  return ++id_global;
}

char *doc2str(DynamicJsonDocument *doc) {
  char *buf = JSON_BUFFER_STR;
  serializeJson(*doc, buf, SPICEAPI_WRAPPER_BUFFER_SIZE_STR);
  return buf;
}

DynamicJsonDocument *request_gen(const char *module, const char *function) {

  // create document
  auto doc = new DynamicJsonDocument(SPICEAPI_WRAPPER_BUFFER_SIZE);

  // add attributes
  (*doc)["id"] = msg_gen_id();
  (*doc)["module"] = module;
  (*doc)["function"] = function;

  // add params
  (*doc).createNestedArray("params");

  // return document
  return doc;
}

DynamicJsonDocument *response_get(Connection &con, const char *json) {

  // parse document
  DynamicJsonDocument *doc = new DynamicJsonDocument(SPICEAPI_WRAPPER_BUFFER_SIZE);
  auto err = deserializeJson(*doc, (char *)json);

  // check for parse error
  if (err) {
    delete doc;
    return nullptr;
  }

  // check id
  if (!(*doc)["id"].is<int64_t>()) {
    delete doc;
    return nullptr;
  }

  // check errors
  auto errors = (*doc)["errors"];
  if (!errors.is<JsonArray>()) {
    delete doc;
    return nullptr;
  }

  // check error count
  if (errors.as<JsonArray>().size() > 0) {
    delete doc;
    return nullptr;
  }

  // check data
  if (!(*doc)["data"].is<JsonArray>()) {
    delete doc;
    return nullptr;
  }

  // return document
  return doc;
}

bool card_insert(Connection &con, size_t index, const char *card_id) {
  auto req = request_gen("card", "insert");
  auto params = (*req)["params"].as<JsonArray>();
  params.add(index);
  params.add(card_id);
  auto req_str = doc2str(req);
  delete req;
  auto res = response_get(con, con.request(req_str));
  if (!res)
    return false;
  delete res;
  return true;
}

bool info_avs(Connection &con, InfoAvs &info) {
  auto req = request_gen("info", "avs");
  auto req_str = doc2str(req);
  delete req;
  auto res = response_get(con, con.request(req_str));
  if (!res)
    return false;
  auto data = (*res)["data"][0];
  info.model = (const char *)data["model"];
  info.dest = (const char *)data["dest"];
  info.spec = (const char *)data["spec"];
  info.rev = (const char *)data["rev"];
  info.ext = (const char *)data["ext"];
  delete res;
  return true;
}
}

#endif  //SPICEAPI_WRAPPERS_H
