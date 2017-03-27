// Copyright 2016 Mobvoi Inc. All Rights Reserved.
// Author: xjliu@mobvoi.com (Xiaojia Liu)

#include "push/serving/train/train_data_fetcher.h"

#include "base/log.h"
#include "crawler/proto/crawldoc.pb.h"
#include "crawler/doc_handler/doc_processor.h"
#include "util/net/http_client/http_client.h"
#include "util/parser/template_parser/template_parser.h"

DECLARE_string(mysql_config);

namespace train {

TrainDataFetcher::TrainDataFetcher() {
  LOG(INFO) << "Construct TrainDataFetcher";
}

TrainDataFetcher::~TrainDataFetcher() {}

bool TrainDataFetcher::FetchTimeTable(const string& url, 
                                      const string& template_file,
                                      Json::Value* result) {
  util::HttpClient http_client;
  http_client.Reset();
  if (!http_client.FetchUrl(url) || http_client.response_code() != 200) {
    LOG(ERROR) << "Fail to fetch url:" << url;
    return false;
  }
  crawl::CrawlDoc crawl_doc;
  crawl_doc.Clear();
  crawl_doc.set_url(url);
  crawl_doc.mutable_response()->set_header(http_client.ResponseHeader());
  crawl_doc.mutable_response()->set_content(http_client.ResponseBody());
  crawl::DocProcessor doc_processor;
  doc_processor.ParseDoc(&crawl_doc);
  string page_content = crawl_doc.response().content();
  LOG(INFO) << "fetched page content size:" << page_content.size();

  return ParsePageContent(url, template_file, page_content, result);
}

bool TrainDataFetcher::ParsePageContent(const string& url, 
                                        const string& template_file,
                                        const string& page_content, 
                                        Json::Value* result) {
  util::HtmlParser html_parser;
  util::TemplateParser template_parser(template_file, &html_parser);
  template_parser.PreProcess(url, 0);
  html_parser.Parse(page_content);
  if (!template_parser.Parse(result)) {
    LOG(ERROR) << "Fail to parse doc with template:" << template_file;
    return false;
  }
  LOG(INFO) << result->toStyledString();
  return true;
}

}  // namespace train 
