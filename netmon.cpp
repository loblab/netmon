// Copyright 2019 loblab
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

#include <chrono>
#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include <thread>
#include <climits>
#include <getopt.h>

using namespace std;
using namespace std::chrono;

typedef long long int TValue;
typedef long long int TTime;
typedef list<string> Strings;

namespace NWM {

int g_debug = 0;

void trace(const char* msg)
{
	if (g_debug > 3)
		cout << msg << endl;
}

typedef TValue (*TAggregator)(TValue* items, int count);

TValue sum(TValue* items, int count)
{
	TValue s = 0;
	for (int i = 0; i < count; i++)
		s += items[i];
	return s;
}

TValue avg(TValue* items, int count)
{
	return sum(items, count) / count;
}

TValue max(TValue* items, int count)
{
	TValue m = 0;
	for (int i = 0; i < count; i++)
		if (m < items[i])
			m = items[i];
	return m;
}

TValue min(TValue* items, int count)
{
	TValue m = LLONG_MAX;
	for (int i = 0; i < count; i++)
		if (m > items[i])
			m = items[i];
	return m;
}

TValue first(TValue* items, int count)
{
	return items[0];
}

TValue last(TValue* items, int count)
{
	return items[count - 1];
}

} // namespace NWM

class Counter
{
private:
	const char* PATH = "/sys/class/net/%s/statistics/%s";

private:
	string m_name;
	string m_prop;
	TValue m_base;
	ifstream m_file;

public:
    Counter(const string& nic, const string& field)
	: m_base(0)
	{
		cout << "Monitor: " << nic << " - " << field << endl;
        m_name = nic;
        m_prop = field;
		m_name += "-";
		m_name += field;
		char path[128];
		snprintf(path, sizeof path, PATH, nic.c_str(), field.c_str());
		if (NWM::g_debug > 2)
			cout << "path: " << path << endl;
		m_file.open(path);
	}

	~Counter()
	{
		m_file.close();
	}

	TValue rebase()
	{
		m_base = 0;
		m_base = getValue();
	}

	const string& getName()
	{
		return m_name;
	}

    TValue getValue()
	{
		const int size = 128;
		char buffer[size];
		m_file.clear();
		m_file.seekg(0, ios::beg);
		m_file.read(buffer, size);
		int len = m_file.gcount() - 1;
		buffer[len] = 0;
		TValue v = stoll(buffer);
		TValue r = v - m_base;
		m_base = v;
		return r;
	}

}; //class Counter

typedef list<Counter*> Counters;

class Monitor
{
private:
	time_point<system_clock> m_t1;
	Counters m_counters;
	TValue* m_values;
	ofstream m_ofs;
	int m_loop;
	bool m_start;

	int m_cycle;
	int m_duration;
	int m_threshold;
	NWM::TAggregator m_func;

public:
	Monitor()
	: m_values(nullptr)
	, m_loop(0)
	, m_start(true)
	{
	}

	~Monitor()
	{
		if (m_values)
		{
			delete[] m_values;
			m_values = nullptr;
		}

		for (Counters::iterator it = m_counters.begin(); it != m_counters.end(); it++)
		{
			Counter* item = *it;
			delete item;
		}
	}

	TTime getTime()
	{
		auto t2 = system_clock::now();
		auto diff = duration_cast<microseconds>(t2 - m_t1);
		return diff.count();
	}

	void addNIC(const string& nic, Strings items)
	{
		for (Strings::iterator it = items.begin(); it != items.end(); it++)
		{
			Counter* c;
			c = new Counter(nic, *it);
			m_counters.push_back(c);
		}
	}

	void setCycle(int cycle)
	{
		cout << "Sample cycle: " << cycle << "(µs)" << endl;
		m_cycle = cycle;
	}

	void setDuration(int dur)
	{
		cout << "Sample duration: " << dur << "(µs)" << endl;
		m_duration = dur;
	}

	void setTrigger(int th, const string& func)
	{
		cout << "Trigger function: " << func << endl;
		cout << "Trigger threshold: " << th << endl;
		m_threshold = th;
		if (func == "min")
			m_func = NWM::min;
		else if (func == "max")
			m_func = NWM::max;
		else if (func == "sum")
			m_func = NWM::sum;
		else if (func == "avg")
			m_func = NWM::avg;
		else if (func == "first")
			m_func = NWM::first;
		else if (func == "last")
			m_func = NWM::last;
		else
		{
			cout << "Warning: unknown function: '" << func << "', use 'max' instead" << endl;
			m_func = NWM::max;
		}
	}

	void setOutput(const string& filepath)
	{
		cout << "Output data file: " << filepath << endl;
		m_ofs.open(filepath, ofstream::out);
	}

	void rebase()
	{
		for (Counters::iterator it = m_counters.begin(); it != m_counters.end(); it++)
		{
			Counter* item = *it;
			item->rebase();
		}
	}

	void sample()
	{
		int i = 0;
		for (Counters::iterator it = m_counters.begin(); it != m_counters.end(); it++)
		{
			Counter* item = *it;
			m_values[i++] = item->getValue();
		}
	}

	void report_header()
	{
		m_ofs << "Time";
		for (Counters::iterator it = m_counters.begin(); it != m_counters.end(); it++)
		{
			Counter* item = *it;
			m_ofs << "," << item->getName();
		}
		m_ofs << endl;
	}

	void report()
	{
		int size = m_counters.size();
		m_ofs << getTime();
		for (int i = 0; i < size; i++)
		{
			m_ofs << "," << m_values[i];
		}
		m_ofs << endl;
	}

	bool check_start()
	{
		if (m_start)
			return true;
		TValue agr = m_func(m_values, m_counters.size());
		if (agr > m_threshold)
		{
			m_start = true;
			m_t1 = system_clock::now();
			m_loop = 0;
		}
		return m_start;
	}

	int run()
	{
		int size = m_counters.size();
		m_values = new TValue[size];

		report_header();

		int total = m_duration / m_cycle;
		int i = 0;
		m_loop = 0;
		rebase();
		m_start = false; // trigger reporting by activity
		m_t1 = system_clock::now();
		while (!m_start || i < total)
		{
			sample();
			if (check_start())
			{
				report();
				i++;
			}
			++m_loop;
			auto t2 = m_t1 + chrono::microseconds(m_cycle * m_loop);
			auto now = system_clock::now();
			if (t2 > now)
				this_thread::sleep_until(t2);
		}
		return 0;
	}

}; //class Monitor

class App
{
protected:
	Monitor m_monitor;

protected:
	static const char* version_info;
	static const char* help_msg;
	static const char* short_options;
	static const struct option long_options[];

    struct
    {
        int debug;
		bool absolute;
		string output;
		int cycle;
		int duration;
		int threshold;
		string aggregator;
		Strings counters;
		Strings nics;
    } m_args;

protected:

	void help()
	{
		cout << help_msg << endl;
	}

	void version()
	{
		cout << version_info << endl;
	}

	void list()
	{
		cout << "TODO" << endl;
	}

	int parseArguments(int argc, char **argv);

	int init()
	{
		for (Strings::iterator it = m_args.nics.begin(); it != m_args.nics.end(); it++)
		{
			m_monitor.addNIC(*it, m_args.counters);
		}
		m_monitor.setCycle(m_args.cycle);
		m_monitor.setDuration(m_args.duration);
		m_monitor.setTrigger(m_args.threshold, m_args.aggregator);
		m_monitor.setOutput(m_args.output);
		return 0;
	}

	void done()
	{
		cout << "Done." << endl;
	}

	int run()
	{
		cout << "Go!" << endl;
		return m_monitor.run();
	}

public:
	App()
	{
        m_args.debug = 0;
		m_args.absolute = false;
		m_args.output = "netmon.csv";
		m_args.cycle = 200;
		m_args.duration = 1000000;
		m_args.threshold = 1000;
		m_args.aggregator = "max";
	}

	int main(int argc, char **argv)
	{
		int rc = parseArguments(argc, argv);
		if (rc)
			return rc;
		NWM::g_debug = m_args.debug;
		rc = init();
		if (rc)
			return rc;
		rc = run();
		done();
		return rc;
	}

}; //class App


const char* App::version_info = "nwmon ver 0.1 (1/15/2019), loblab";

const char* App::help_msg =
	"High precision network monitor\n"
	"Usage: nwmon [options] [nic1] [nic2] ...\n"
	"Options:\n"
	"    -h|--help        : help message\n"
	"    -v|--version     : version info\n"
	"    -l|--list        : list avaiable counters\n"
	"    -c|--cycle     n : sample cycle, in microsecond\n"
	"    -d|--duration  n : sample duration, in microsecond\n"
	"    -f|--function  s : trigger aggregator function, can be min, max, sum, arg, first, last\n"
	"    -t|--threshold n : trigger level, if function(counters) > threshold, start recording\n"
	"    -o|--output    s : output file, default: netmon.csv\n"
	"    -n|--counter   s : add a counter. to add multiple: -c couter1 -c counter2\n"
	"    -D|--debug     n : debug info level. default 0 for none, greater for more\n"
	//"    -a|--absolute    : output absolute time. default relative timestamp, i.e. start from 0\n"
	"nic1, nic2...:\n"
	"    network interface names\n"
;

const char* App::short_options = "hvlac:d:D:t:o:n:f:";

const struct option App::long_options[] =
{
	{"help",     no_argument,       0, 'h'},
	{"version",  no_argument,       0, 'v'},
	{"list",     no_argument,       0, 'l'},
	{"absolute", no_argument,       0, 'a'},
	{"debug",    required_argument, 0, 'D'},
	{"cycle",    required_argument, 0, 'c'},
	{"duration", required_argument, 0, 'd'},
	{"threshold",required_argument, 0, 't'},
	{"function", required_argument, 0, 'f'},
	{"output",   required_argument, 0, 'o'},
	{"counter",  required_argument, 0, 'n'},
	{0, 0, 0, 0}
};

int App::parseArguments(int argc, char **argv)
{
	while (1)
	{
		// getopt_long stores the option index here.
		int option_index = 0;
		int c = getopt_long(argc, argv, short_options, long_options, &option_index);
		// Detect the end of the options.
		if (c == -1)
			break;

		switch (c)
		{
		case 0:
			// If this option set a flag, do nothing else now.
			if (long_options[option_index].flag != 0)
				break;
			cout << "option " << long_options[option_index].name;
			if (optarg)
				cout << " with arg " << optarg;
			cout << endl;
			break;

		case 'h':
			help();
			exit(0);

		case 'v':
			version();
			exit(0);

		case 'l':
			list();
			exit(0);

		case 'a':
			m_args.absolute = true;
			break;

		case 'c':
			m_args.cycle = atoi(optarg);
			break;

		case 'd':
			m_args.duration = atoi(optarg);
			break;

		case 't':
			m_args.threshold = atoi(optarg);
			break;

		case 'o':
			m_args.output = optarg;
			break;

		case 'f':
			m_args.aggregator = optarg;
			break;

		case 'n':
			m_args.counters.push_back(optarg);
			break;

		case 'D':
			m_args.debug = atoi(optarg);
			if (m_args.debug >= 9)
				cout << "Debug level: " << m_args.debug << endl;
			break;

		case '?':
			//invalid options return as '?'
			// getopt_long already printed an error message.
			cout << "Invalid options" << endl;
			help();
			return -1;

		default:
			help();
			return -1;
		}
	}

	// non-option ARGV items
	int item_count = argc - optind;
	if (m_args.debug >= 9)
		cout << "Positional argument count: " << item_count << endl;
	while (optind < argc)
		m_args.nics.push_back(argv[optind++]);

	return 0;
}


int main (int argc, char** argv)
{
	App app;
    return app.main(argc, argv);
}

