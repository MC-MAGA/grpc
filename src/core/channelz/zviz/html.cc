// Copyright 2025 The gRPC Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/core/channelz/zviz/html.h"

#include "absl/functional/function_ref.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"

namespace grpc_zviz::html {

std::string HtmlEscape(absl::string_view text) {
  return absl::StrReplaceAll(text, {{"&", "&amp;"},
                                    {"<", "&lt;"},
                                    {">", "&gt;"},
                                    {"\"", "&quot;"},
                                    {"'", "&apos;"}});
}

Container Div(std::string clazz, absl::FunctionRef<void(Container&)> f) {
  Container div("div");
  div.Attribute("class", std::move(clazz));
  f(div);
  return div;
}

std::string Container::Render() const {
  std::string s;
  if (tag_.has_value()) {
    absl::StrAppend(&s, "<", *tag_);
    for (const auto& [name, value] : attributes_) {
      absl::StrAppend(&s, " ", name, "=\"", HtmlEscape(value), "\"");
    }
    if (items_.empty()) {
      absl::StrAppend(&s, "/>");
      return s;
    }
    absl::StrAppend(&s, ">");
  }
  for (const auto& item : items_) {
    absl::StrAppend(&s, item->Render());
  }
  if (tag_.has_value()) {
    absl::StrAppend(&s, "</", *tag_, ">");
  }
  return s;
}

Container& Container::Link(std::string text, std::string url) {
  NewItem<Container>("a")
      .Attribute("href", std::move(url))
      .Text(std::move(text));
  return *this;
}

Container& Container::Div(std::string clazz,
                          absl::FunctionRef<void(Container&)> f) {
  return NewItem<Container>(html::Div(std::move(clazz), f));
}

Container& Container::NewDiv(std::string clazz) {
  auto& div = NewItem<Container>("div");
  div.Attribute("class", std::move(clazz));
  return div;
}

Container& Container::AddStyle(absl::string_view style) {
  NewItem<Container>("style").Text(std::string(style));
  return *this;
}

Table& Container::NewTable(std::string clazz) {
  return NewItem<Table>(std::move(clazz));
}

Container& Table::Cell(int column, int row) {
  num_columns_ = std::max(num_columns_, column + 1);
  num_rows_ = std::max(num_rows_, row + 1);
  auto it = cells_.find(std::tuple(column, row));
  if (it == cells_.end()) {
    it = cells_.emplace(std::tuple(column, row), Container("div")).first;
  }
  return it->second;
}

std::string Table::Render() const {
  std::string s = absl::StrCat("<table class=\"", HtmlEscape(clazz_), "\">");

  int r = 0;
  if (num_header_rows_ > 0) {
    absl::StrAppend(&s, "<thead>");
    for (; r < num_header_rows_; ++r) {
      absl::StrAppend(&s, "<tr>");
      for (int c = 0; c < num_columns_; ++c) {
        auto it = cells_.find(std::tuple(c, r));
        if (it == cells_.end()) {
          absl::StrAppend(&s, "<th/>");
        } else {
          absl::StrAppend(&s, "<th>", it->second.Render(), "</th>");
        }
      }
      absl::StrAppend(&s, "</tr>");
    }
    absl::StrAppend(&s, "</thead>");
  }

  absl::StrAppend(&s, "<tbody>");
  if (r < num_rows_) {
    for (; r < num_rows_; ++r) {
      absl::StrAppend(&s, "<tr>");
      for (int c = 0; c < num_columns_; ++c) {
        auto it = cells_.find(std::tuple(c, r));
        const char* tag = c < num_header_columns_ ? "th" : "td";
        if (it == cells_.end()) {
          absl::StrAppend(&s, "<", tag, "/>");
        } else {
          absl::StrAppend(&s, "<", tag, ">", it->second.Render(), "</", tag,
                          ">");
        }
      }
      absl::StrAppend(&s, "</tr>");
    }
  }
  absl::StrAppend(&s, "</tbody>");

  absl::StrAppend(&s, "</table>");
  return s;
}

}  // namespace grpc_zviz::html
