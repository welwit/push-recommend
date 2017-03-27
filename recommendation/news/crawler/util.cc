// Copyright 2017 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "recommendation/news/crawler/util.h"

#include "base/log.h"
#include "crawler/doc_handler/doc_processor.h"
#include "util/net/http_client/http_client.h"
#include "util/parser/template_parser/template_parser.h"

DECLARE_int32(fetch_timeout_ms);

namespace recommendation {

bool FetchHtml(const string& url, string* html) {
  VLOG(1) << "Url:" << url;
  util::HttpClient http_client;
  http_client.SetFetchTimeout(FLAGS_fetch_timeout_ms);
  http_client.FetchUrl(url);
  if (http_client.response_code() == 200) {
    crawl::CrawlDoc doc;
    doc.set_url(url);
    doc.mutable_response()->set_header(http_client.ResponseHeader());
    doc.mutable_response()->set_content(http_client.ResponseBody());
    crawl::DocProcessor doc_processor;
    doc_processor.ParseDoc(&doc);
    html->assign(doc.response().content());
    return true;
  } else {
    LOG(WARNING) << "Fail to fetch Url:" << url;
    return false;
  }
}

bool FetchHtmlSimple(const string& url, string* html) {
  VLOG(1) << "Url:" << url;
  util::HttpClient http_client;
  http_client.SetFetchTimeout(FLAGS_fetch_timeout_ms);
  http_client.FetchUrl(url);
  if (http_client.response_code() == 200) {
    html->assign(http_client.ResponseBody());
    return true;
  } else {
    LOG(WARNING) << "Fail to fetch Url:" << url;
    return false;
  }
}

void ParseHtml(const string& html, const string& templatefile,
               Json::Value* result) {
  VLOG(1) << "templatefile:" << templatefile;
  util::HtmlParser html_parser;
  util::TemplateParser template_parser(templatefile, &html_parser);
  template_parser.PreProcess("", 0);
  html_parser.Parse(html);
  result->clear();
  template_parser.Parse(result);
}

bool FetchToJson(const string& url,
                 const string& templatefile, Json::Value* result) {
  string html;
  if (!FetchHtml(url, &html)) {
    return false;
  }
  VLOG(1) << "Good Html:" << html;
  ParseHtml(html, templatefile, result);
  return true;
}

void ParseLinkData(const Json::Value& link_data, vector<string>* link_vec) {
  Json::Value::Members names = link_data.getMemberNames();
  for (auto& name : names) {
    for (Json::Value::ArrayIndex i = 0; i < link_data[name].size(); ++i) {
      const Json::Value& link_item = link_data[name][i];
      string link = link_item["link"].asString();
      link_vec->push_back(link);
    }
  }
}

void ParseLinkToJson(const Json::Value& link_data,
                     vector<Json::Value>* data_vec) {
  Json::Value::Members names = link_data.getMemberNames();
  for (auto& name : names) {
    for (Json::Value::ArrayIndex i = 0; i < link_data[name].size(); ++i) {
      const Json::Value& link_item = link_data[name][i];
      data_vec->push_back(link_item);
    }
  }
}

bool IsValidLinkResult(const Json::Value& link_result) {
  if (link_result.isMember("data")) {
    return true;
  }
  VLOG(1) << "Invalid link result";
  return false;
}

bool IsValidLink(const Json::Value& link) {
  if (link.isMember("link") &&
      !link["link"].empty()) {
    return true;
  }
  VLOG(1) << "Invalid link";
  return false;
}

bool IsValidNewsContent(const Json::Value& input) {
  if (input.isMember("data") &&
      input["data"].isMember("title") && input["data"].isMember("content") &&
      !input["data"]["title"].empty() && !input["data"]["content"].empty()) {
    return true;
  }
  VLOG(1) << "Invalid news content";
  return false;
}

bool FetchNewsContent(const string& url, const string& templatefile,
                      Json::Value* content) {
  if (FetchToJson(url, templatefile, content) &&
      IsValidNewsContent(*content)) {
    (*content)["data"]["url"] = url;
    VLOG(1) << "Template file: " << templatefile << ", Content:" 
            << content->toStyledString();
    return true;
  }
  return false;
}

bool FetchNewsContent(const string& url,
                      const vector<string>& templates,
                      Json::Value* content) {
  for (auto& tpl : templates) {
    if (FetchNewsContent(url, tpl, content)) {
      return true;
    }
  }
  return false;
}

}  // namespace recommendation
