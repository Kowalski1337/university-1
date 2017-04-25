#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>

enum class token_type
{
	OR, XOR, AND, NOT,
	VAR, LPAREN, RPAREN, END
};

struct token
{
	token_type type_;
	std::string str_;

	token(token_type type, std::string str)
		: type_{type}
		, str_{std::move(str)}
	{ }
};

bool str_cmp(std::string const &str, size_t str_i, std::string const &other)
{
	return str_i + other.size() <= str.size() &&
			std::equal(other.begin(), other.end(), str.begin() + str_i);
}

void skip_spaces(std::string const *str, size_t *i)
{
	while (*i < str->size() && isspace(str->at(*i)))
		++*i;
}

std::vector<token> tokenize(std::string const &str)
{
	std::vector<token> const special_tokens =
		{{token_type::OR,     "or"},
		 {token_type::XOR,    "xor"},
		 {token_type::AND,    "and"},
		 {token_type::NOT,    "not"},
		 {token_type::LPAREN, "("},
		 {token_type::RPAREN, ")"}};

	std::vector<token> tokens;

	size_t i = 0;
	skip_spaces(&str, &i);
	while (i < str.size())
	{
		bool is_special = false;
		for (auto const &special : special_tokens)
			if (str_cmp(str, i, special.str_))
			{
				tokens.push_back(special);
				i += special.str_.size();
				is_special = true;
				break;
			}

		if (!is_special)
		{
			if (isalpha(str[i]))
			{
				tokens.emplace_back(token_type::VAR, std::string{str[i]});
				++i;
			}
			else
				throw std::runtime_error{std::string("unknown char ") + str[i]};
		}

		skip_spaces(&str, &i);
	}

	tokens.emplace_back(token_type::END, "");

	return tokens;
}

struct node
{
	std::string str_;
	std::vector<node> children_;

	node(std::string str)
		: str_{str}
	{ }
};

struct parser
{
	std::vector<token> const *tokens_;
	size_t i;

	parser(std::vector<token> const *tokens)
		: tokens_{tokens}
	{
		i = 0;
	}

	void consume(token_type const &type)
	{
		if (tokens_->at(i).type_ != type)
			throw std::runtime_error{"unexpected token"};
		++i;
	}

	node parse_d()
	{
		node res{"D"};
		switch (tokens_->at(i).type_)
		{
			case token_type::VAR:
			case token_type::NOT:
			case token_type::LPAREN:
				res.children_.push_back(parse_c());
				res.children_.push_back(parse_d1());
				break;
			default:
				throw std::runtime_error{"unexpected token"};
		}
		return res;
	}

	node parse_d1()
	{
		node res{"D1"};
		switch (tokens_->at(i).type_)
		{
			case token_type::OR:
			case token_type::XOR:
				res.children_.emplace_back(tokens_->at(i).str_);
				consume(tokens_->at(i).type_);
				res.children_.push_back(parse_c());
				res.children_.push_back(parse_d1());
				break;
			case token_type::RPAREN:
			case token_type::END:
				break;
			default:
				throw std::runtime_error{"unexpected token"};
		}
		return res;
	}

	node parse_c()
	{
		node res{"C"};
		switch (tokens_->at(i).type_)
		{
			case token_type::VAR:
			case token_type::NOT:
			case token_type::LPAREN:
				res.children_.push_back(parse_u());
				res.children_.push_back(parse_c1());
				break;
			default:
				throw std::runtime_error{"unexpected token"};
		}
		return res;
	}

	node parse_c1()
	{
		node res{"C1"};
		switch (tokens_->at(i).type_)
		{
			case token_type::AND:
				res.children_.emplace_back(tokens_->at(i).str_);
				consume(tokens_->at(i).type_);
				res.children_.push_back(parse_u());
				res.children_.push_back(parse_c1());
				break;
			case token_type::OR:
			case token_type::XOR:
			case token_type::RPAREN:
			case token_type::END:
				break;
			default:
				throw std::runtime_error{"unexpected token"};
		}
		return res;
	}

	node parse_u()
	{
		node res{"U"};
		switch (tokens_->at(i).type_)
		{
			case token_type::VAR:
				res.children_.emplace_back(tokens_->at(i).str_);
				consume(tokens_->at(i).type_);
				break;
			case token_type::NOT:
				consume(tokens_->at(i).type_);
				res.children_.push_back(parse_u());
				break;
			case token_type::LPAREN:
				consume(token_type::LPAREN);
				res.children_.emplace_back("(");
				res.children_.push_back(parse_d());
				consume(token_type::RPAREN);
				res.children_.emplace_back(")");
				break;
			default:
				throw std::runtime_error{"unexpected token"};
		}
		return res;
	}

	node parse()
	{
		return parse_d();
	}
};

void print_node(std::ostream &out, node const &n, size_t depth = 0)
{
	for (size_t i = 0; i < depth; ++i)
		out << "\t";
	out << n.str_ << "\n";
	for (auto const &child : n.children_)
		print_node(out, child, depth + 1);
}

int main()
{
	try
	{
		std::string s;
		std::getline(std::cin, s);
		auto tokens = tokenize(s);
		node n = parser(&tokens).parse();
		print_node(std::cout, n);
	}
	catch (std::exception const &e)
	{
		std::cout << e.what() << "\n";
	}
	return 0;
}
