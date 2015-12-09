/***
    This file is part of popl (program options parser lib)
    Copyright (C) 2015  Johannes Pohl

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
***/

#ifndef POPL_H
#define POPL_H

#include <getopt.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <stdexcept>

/*
Allowed options:
  -h [ --help ]                         produce help message
  -v [ --version ]                      show version number
  -p [ --port ] arg (=1704)             server port
  --controlPort arg (=1705)             Remote control port
  -s [ --sampleformat ] arg (=44100:16:2)
                                        sample format
  -c [ --codec ] arg (=flac)            transport codec [flac|ogg|pcm][:options
                                        ]. Type codec:? to get codec specific
                                        options
  -f [ --fifo ] arg (=/tmp/snapfifo)    name of the input fifo file
  -d [ --daemon ] [=arg(=-3)]           daemonize, optional process priority
                                        [-20..19]
  -b [ --buffer ] arg (=1000)           buffer [ms]
  --pipeReadBuffer arg (=20)            pipe read buffer [ms]
*/


namespace popl
{

class OptionParser;

class Option
{
friend class OptionParser;
public:
	Option(const std::string& shortOption, const std::string& longOption, const std::string& description);

	char getShortOption() const;
	std::string getLongOption() const;
	std::string getDescription() const;
	unsigned int count() const;


protected:
	virtual void parse(const std::string& value) = 0;
	virtual void update();
	virtual std::string optionToString() const;
	virtual std::vector<std::string> descriptionToString(size_t width = 40) const;
	virtual int hasArg() const = 0;

	std::string shortOption_;
	std::string longOption_;
	std::string description_;
	unsigned int count_;
};


Option::Option(const std::string& shortOption, const std::string& longOption, const std::string& description) :
	shortOption_(shortOption),
	longOption_(longOption),
	description_(description),
	count_(0)
{
}


void Option::parse(const std::string& value)
{
}


void Option::update()
{
}


char Option::getShortOption() const
{
	if (!shortOption_.empty())
		return shortOption_[0];
	return 0;
}


std::string Option::getLongOption() const
{
	return longOption_;
}


std::string Option::getDescription() const
{
	return description_;
}


unsigned int Option::count() const
{
	return count_;
}


std::string Option::optionToString() const
{
	std::stringstream line;
	if (getShortOption() != 0)
	{
		line << "  -" << getShortOption();
		if (!getLongOption().empty())
			line << ", ";
	}
	else
		line << "  ";

	if (!getLongOption().empty())
		line << "--" << getLongOption();

	return line.str();
}


std::vector<std::string> Option::descriptionToString(size_t width) const
{
	std::vector<std::string> lines;
	std::stringstream description(getDescription());
	std::string line;
	while (std::getline(description, line, '\n'))
		lines.push_back(line);

	return lines;
}


template<class T>
class Value : public Option
{
public:
	Value(const std::string& shortOption, const std::string& longOption, const std::string& description);
	Value(const std::string& shortOption, const std::string& longOption, const std::string& description, const T& defaultVal);
	Value(const std::string& shortOption, const std::string& longOption, const std::string& description, const T& defaultVal, T& assignTo);
	Value<T>& assignTo(T& var);
	Value<T>& setDefault(const T& value);
	T getValue() const;

protected:
	virtual void parse(const std::string& value);
	virtual std::string optionToString() const;
	virtual void update();
	virtual int hasArg() const;
	T* assignTo_;
	T value_;
	bool hasDefault_;
};


//TODO
//class Implicit


template<class T>
Value<T>::Value(const std::string& shortOption, const std::string& longOption, const std::string& description) : Option(shortOption, longOption, description), assignTo_(NULL), hasDefault_(false)
{
	update();
}


template<class T>
Value<T>::Value(const std::string& shortOption, const std::string& longOption, const std::string& description, const T& defaultVal) : Option(shortOption, longOption, description), assignTo_(NULL), value_(defaultVal), hasDefault_(true)
{
	update();
}


template<class T>
Value<T>::Value(const std::string& shortOption, const std::string& longOption, const std::string& description, const T& defaultVal, T& assignTo) : Option(shortOption, longOption, description), assignTo_(&assignTo), value_(defaultVal), hasDefault_(true)
{
	update();
}


template<class T>
Value<T>& Value<T>::assignTo(T& var)
{
	assignTo_ = &var;
	return *this;
}


template<class T>
Value<T>& Value<T>::setDefault(const T& value)
{
	value_ = value;
	hasDefault_ = true;
	return *this;
}


template<class T>
void Value<T>::update()
{
	if (assignTo_ != NULL)
		*assignTo_ = value_;
}


template<class T>
T Value<T>::getValue() const
{
	return value_;
}


template<class T>
int Value<T>::hasArg() const
{
	return required_argument;
}


template<>
void Value<std::string>::parse(const std::string& value)
{
	value_ = value;
	update();
}


template<class T>
void Value<T>::parse(const std::string& value)
{
	std::istringstream is(value);
	int valuesRead = 0;
	while (is.good())
	{
		if (is.peek() != EOF)
			is >> value_;
		else
			break;

		valuesRead++;
	}

	update();

	if (is.fail() || (valuesRead > 1))
	{
		std::stringstream option;
		if (getShortOption() != 0)
			option << "-" << getShortOption();
		else
			option << "--" << getLongOption();

		if (is.fail())
			throw std::invalid_argument("invalid argument for " + option.str() + ": '" + value + "'");

		if (valuesRead > 1)
			throw std::invalid_argument("too many arguments for " + option.str() + ": '" + value + "'");
	}
}


template<class T>
std::string Value<T>::optionToString() const
{
	std::stringstream ss;
	ss << Option::optionToString() << " arg";
	if (hasDefault_)
		ss << " (=" << value_ << ")";
	return ss.str();
}



class Switch : public Value<bool>
{
public:
	Switch(const std::string& shortOption, const std::string& longOption, const std::string& description);
	Switch(const std::string& shortOption, const std::string& longOption, const std::string& description, bool& assignTo);

protected:
	Switch& setDefault(const bool& value);
	virtual void parse(const std::string& value);
	virtual int hasArg() const;
};


Switch::Switch(const std::string& shortOption, const std::string& longOption, const std::string& description) : Value<bool>(shortOption, longOption, description, false)
{
	update();
}


Switch::Switch(const std::string& shortOption, const std::string& longOption, const std::string& description, bool& assignTo) : Value<bool>(shortOption, longOption, description, false, assignTo)
{
	update();
}


void Switch::parse(const std::string& value)
{
	value_ = true;
	update();
}


int Switch::hasArg() const
{
	return no_argument;
}





class OptionParser
{
public:
	OptionParser(const std::string& description = "");
	virtual ~OptionParser();
	OptionParser& add(Option& option);
	OptionParser& add(Option* option);
	void parse(int argc, char **argv);
	std::string help() const;
//TODO
//non options
//unknown options

protected:
	std::vector<Option*> options_;
	std::string description_;
};


OptionParser::OptionParser(const std::string& description) : description_(description)
{
}


OptionParser::~OptionParser()
{
}


OptionParser& OptionParser::add(Option& option)
{
	options_.push_back(&option);
	return *this;
}


std::string OptionParser::help() const
{
	std::stringstream s;
	if (!description_.empty())
		s << description_ << ":\n";

	size_t optionRightMargin(20);
	const size_t maxDescriptionLeftMargin(40);
//	const size_t descriptionRightMargin(80);

	for (size_t opt = 0; opt < options_.size(); ++opt)
		optionRightMargin = std::max(optionRightMargin, options_[opt]->optionToString().size() + 2);
	optionRightMargin = std::min(maxDescriptionLeftMargin - 2, optionRightMargin);

	for (size_t opt = 0; opt < options_.size(); ++opt)
	{
		std::string optionStr = options_[opt]->optionToString();
		if (optionStr.size() < optionRightMargin)
			optionStr.resize(optionRightMargin, ' ');
		else
			optionStr += "\n" + std::string(optionRightMargin, ' ');
		s << optionStr;

		std::vector<std::string> lines = options_[opt]->descriptionToString(20);
		std::string empty(optionRightMargin, ' ');
		for (size_t n=0; n<lines.size(); ++n)
		{
			if (n > 0)
				s << "\n" << empty;
			s << lines[n];
		}
		s << "\n";
	}

	return s.str();
}


void OptionParser::parse(int argc, char **argv)
{
	std::vector<option> long_options;
	std::stringstream short_options;
	opterr = 0;
	for (size_t opt = 0; opt < options_.size(); ++opt)
	{
		Option* option(options_[opt]);
		option->update();
		if (!option->getLongOption().empty())
		{
			::option o;
			o.name = option->getLongOption().c_str();
			o.has_arg = option->hasArg();
			o.flag = 0;
			o.val = option->getShortOption();
			long_options.push_back(o);
		}
		if (option->getShortOption() != 0)
		{
			short_options << option->getShortOption();
			if (option->hasArg() == required_argument)
				short_options << ":";
			else if (option->hasArg() == optional_argument)
				short_options << "::";
		}
	}

	while (1)
	{
		int option_index = 0;
		int curind = optind;
		char c = getopt_long(argc, argv, short_options.str().c_str(), &long_options[0], &option_index);
		if (c == -1)
			break;

		Option* option(NULL);
		if (c == 0)
		{
			for (size_t opt = 0; opt < options_.size(); ++opt)
			{
				if (strcmp(options_[opt]->getLongOption().c_str(), long_options[option_index].name) == 0)
				{
					option = options_[opt];
					break;
				}
			}
		}
		else if (c != '?')
		{
//std::cout << c << ", " << (char)c << "\n";
			for (size_t opt = 0; opt < options_.size(); ++opt)
			{
				if (options_[opt]->getShortOption() == c)
				{
					option = options_[opt];
					break;
				}
			}
		}
		else // ?
		{
std::cout << "unknown: " << c << ", " << (char)c << ", " << optopt << ", " << (char)optopt << ", " << argv[curind] << "\n";
		}

		if (option != NULL)
		{
			++option->count_;
//			std::cout << option->getDescription() << ": " << optarg << "\n";
			if (optarg != NULL)
				option->parse(optarg);
			else
				option->parse("");
		}
	}

	if (optind < argc)
	{
		std::cout << "non-option ARGV-elements: ";
		while (optind < argc)
			std::cout << argv[optind++] << " ";
		std::cout << "\n";
	}
}


std::ostream& operator<<(std::ostream& out, const OptionParser& op)
{
    out << op.help();
    return out;
}


} // namespace popl

#endif


