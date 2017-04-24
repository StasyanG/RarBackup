typedef std::codecvt<wchar_t, char, mbstate_t> null_wcodecvt_base;
class null_wcodecvt : public null_wcodecvt_base
{
public:
	explicit null_wcodecvt(size_t refs = 0) : null_wcodecvt_base(refs) {}

protected:
	virtual result do_out(mbstate_t&,
		const wchar_t* from,
		const wchar_t* from_end,
		const wchar_t*& from_next,
		char* to,
		char* to_end,
		char*& to_next) const
	{
		size_t len = (from_end - from) * sizeof(wchar_t);
		memcpy(to, from, len);
		from_next = from_end;
		to_next = to + len;
		return ok;
	}//do_out

	virtual result do_in(mbstate_t&,
		const char* from,
		const char* from_end,
		const char*& from_next,
		wchar_t* to,
		wchar_t* to_end,
		wchar_t*& to_next) const
	{
		size_t len = (from_end - from);
		memcpy(to, from, len);
		from_next = from_end;
		to_next = to + (len / sizeof(wchar_t));
		return ok;
	}//do_in

	virtual result do_unshift(mbstate_t&, char* to, char*,
		char*& to_next) const
	{
		to_next = to;
		return noconv;
	}//do_unshift

	virtual int do_length(mbstate_t&, const char* from,
		const char* end, size_t max) const
	{
		return (int)((max < (size_t)(end - from)) ? max : (end - from));
	}//do_length

	virtual bool do_always_noconv() const throw()
	{
		return true;
	}//do_always_noconv

	virtual int do_encoding() const throw()
	{
		return sizeof(wchar_t);
	}//do_encoding

	virtual int do_max_length() const throw()
	{
		return sizeof(wchar_t);
	}//do_max_length
};//null_wcodecvt
const wchar_t UTF_BOM = 0xfeff;