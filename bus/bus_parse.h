#include <stdint.h>
#include <stdio.h>
#include <string.h>

// :TODO: handle non-standard times (17:38)
const char* g_pStartString = "<td class=\"body-cell\" align=\"right\">";
size_t g_startSize = strlen(g_pStartString);

const char* g_pNowString = "</span> at <span class=\"emphasise\">";
size_t g_nowSize = strlen(g_pNowString);

const char* g_pEndString = "</td><td";
size_t g_endSize = strlen(g_pEndString);

struct RollingBuffer
{
	void init();
	void add(char ch);
	bool match(const char* pString, uint16_t len);

	static const uint16_t kBufSize = 100;

	char			m_buf[kBufSize];
	uint16_t		m_inBuf;			// Number of entries in m_buf that are valid
	uint16_t		m_nextOff;			// Where next character will be written
};

void RollingBuffer::init()
{
	m_inBuf = 0;
	m_nextOff = 0;
}

void RollingBuffer::add(char ch)
{
	m_buf[m_nextOff] = ch;
	m_nextOff = (m_nextOff + 1U) % kBufSize;
	if (m_inBuf < kBufSize)
		++m_inBuf;
}

bool RollingBuffer::match(const char* pString, uint16_t len)
{
	if (len > kBufSize)
		return false;

	// Find the first position
	// Last char added is at m_nextOff, which should match (-len)
	for (uint16_t off = 0U; off < len; ++off)
	{
		// Calc offset in m_buff
		// len = 3 "ABC"
		// e.g. nextOff = 9
		// our string to match would be in entries 6,7,8
		uint16_t bufOff = kBufSize + m_nextOff + off - len;

		// Handle buffer wrap, value will be +ve because of kBufSize above
		bufOff %= kBufSize;
		if (m_buf[bufOff] != pString[off])
			return false;
	}
	return true;
}

struct TimeParser
{
	void init();
	bool add(char ch);

	uint32_t		m_accTime;
	bool			m_lastColon;
	bool			m_absolute;
};

static uint32_t GetTimeDiff(const TimeParser& nowTime, const TimeParser& busTime)
{
	if (busTime.m_absolute)
	{
		if (busTime.m_accTime >= nowTime.m_accTime)
			return busTime.m_accTime - nowTime.m_accTime;
		// Bus time is tomorrow, so add 24 hours
		return 24 * 60 + busTime.m_accTime - nowTime.m_accTime;
	}

	return busTime.m_accTime;
}

void TimeParser::init()
{
	m_accTime = 0U;
	m_lastColon = false;
	m_absolute = false;
}

bool TimeParser::add(char ch)
{
	// Accumulate the time
	if ((ch >= '0') && (ch <= '9'))
	{
		if (m_lastColon)
			m_accTime *= 6U;
		else
			m_accTime *= 10U;

		m_accTime += (ch - '0');
		m_lastColon = false;
		return false;
	}
	else if (ch == ':')
	{
		// Full time. Multiply seconds up
		// Time will already be e.g. "21", so we need to multiply that by 60 for minutes
		// However next time round will multiply by 10...
		m_lastColon = true;
		m_absolute = true;
		return false;
	}
	return true;
}

struct BusParser
{
	void init();
	void add(char ch);
	enum
	{
		kNone,
		kNowTime,
		kBusTime
	} m_inTime;
	
	RollingBuffer	m_buffer;
    uint16_t        m_numTimes;
    TimeParser		m_time[3];
    TimeParser		m_nowTime;
};

void BusParser::init()
{
	m_inTime = kNone;
        m_numTimes = 0U;
	m_buffer.init();
}

void BusParser::add(char ch)
{
	m_buffer.add(ch);

	// Now do state matching
	if (m_inTime == kNone)
	{
		if (m_buffer.match(g_pStartString, g_startSize))
		{
            if (m_numTimes < 3)
            {
				m_time[m_numTimes].init();
				m_inTime = kBusTime;
        	}
		}
		else if (m_buffer.match(g_pNowString, g_nowSize))
		{
			m_nowTime.init();
			m_inTime = kNowTime;
		}
	}
	else if (m_inTime == kBusTime)
	{
		if (m_time[m_numTimes].add(ch))
		{
        	++m_numTimes;
			m_inTime = kNone;
		}
	}
	else if (m_inTime == kNowTime)
	{
		if (m_nowTime.add(ch))
		{
			m_inTime = kNone;
		}
	}
}

static int ParseBlock(BusParser& parse, const char* pBlock, uint32_t blockSize)
{	
	for (uint32_t i = 0; i < blockSize; ++i)
	{
		parse.add(pBlock[i]);
	}
	return 0;
}
