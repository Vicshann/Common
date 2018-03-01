/*
    Copyright (C) 2010, Ferruccio Barletta (ferruccio.barletta@gmail.com)

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use,
    copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following
    conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef XML_WRITER_HPP
#define XML_WRITER_HPP

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <stack>
#include <cassert>

namespace xml {

    class element;

    //
    //	xml::writer class
    //
    class writer
    {
    public:
        // writer must be bound to an ostream
		writer(std::ostream& os) : os(os), need_header(false), indents(0),records(0),format(true) {}
		~writer(void) { this->clear(); }

	public:
		int records;  // total
		int indents;
		bool format;
		std::ostream& os;               // output stream
        bool need_header;               // have we written an XML header yet?
        std::stack<element*> elements;  // stack of open element tags
		void clear(void){elements.empty(); os.flush();}


        // write XML header, if necessary
		writer& header() {
            if (need_header) {
				os << "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
				if(format)os << "\r\n";
				need_header = false;
            }
            return *this;
		}

        // write a single character to the output stream
        writer& xputc(char c) {
            os.put(c);
            return *this;
        }

		// write a string to the output stream
		writer& xputs(const char* str) {
            os << str;
            return *this;
        }
		writer& xindent(void) {
		   char tbuf[128];
		   if(!format)return *this;
		   if(records > 0)os << "\r\n";
		   tbuf[0] = 0;
		   for(int ctr=0,tctr=0;ctr < this->indents;ctr++,tctr++)
			{
			 if(tctr >= (sizeof(tbuf)-1))
			  {
			   tbuf[tctr] = 0;
			   tctr = 0;
			   os << (char*)&tbuf;
			  }
			 tbuf[tctr] = '\t';
			 tbuf[tctr+1] = 0;
			}
			if(tbuf[0])os << (char*)&tbuf;
			return *this;
        }

        // write a string to the output stream & strip out any invalid XML characters
        writer& sxputs(const char* str) {
            for (; *str; ++str)
                switch (*str) {
                case '\r':
                case '\n':
                case '\t':
                    os << *str;
                    break;
                default:
                    if (*str >= ' ')
                        os << *str;
            }
            return *this;
        }

        friend element;
    };

    //
    //	xml::element class
    //
    class element
	{
	 int children;
    public:
		// create a new element tag, bound to an xml::writer
		element(const char* name, writer& wr) : name(name), wr(wr) {
		  //  assert(name != 0);
			children = 0;
			if(!wr.elements.empty())wr.elements.top()->children++;
            check_parent();
			wr.header().xindent().xputc('<').xputs(name);
            tagopen = true;
			wr.elements.push(this);
		 wr.indents++;
		 wr.records++;
		}

        // close the current element tag
		~element() {
			wr.indents--;
			if (!wr.elements.empty() && wr.elements.top() == this) {
				wr.elements.pop();
                if (tagopen)
                    wr.xputs("/>");
				else
				 {
				 // if(strcmp(name,"table")==0)
                 //    name = name;
				  if(this->children > 0)wr.xindent();
				  wr.xputs("</").xputs(name).xputc('>'); // TODO: Real indenting
				 }
			}
        }

        // write an attribute for the current element
		element& attr(const char* name, const char* value) {
		  //  assert(name != 0);
		  //  assert(value != 0);
		  //  assert(tagopen);
			wr.xputc(' ').xputs(name).xputs("=\"");
			qxputs(value);
			wr.xputc('"');
			return *this;
		}
		element& attr_name(const char* name) {      // Write a name part of attribute
			wr.xputc(' ').xputs(name).xputs("=\"");
			return *this;
		}
		element& attr_value(const char* value) {   // // Write a value part of attribute
			qxputs(value);
			wr.xputc('"');
			return *this;
		}

        // attr() overload for std::string type
		//element& attr(const char* name, const std::string& value) { return attr(name, value.c_str()); }

        // attr() function template for all streamable types
		/*template <class T>
        element& attr(const char* name, T value) {
            std::stringstream ss;
            ss << value;
            attr(name, ss.str());
            return *this;
        } */

        // write text content for the current element
        element& contents(const char* str) {
		   // assert(str != 0);
            check_parent();
            qxputs(str);
            return *this;
        }

        // contents() overload for std::string type
		///element& contents(const std::string& str) { return contents(str.c_str()); }

        // contents() function template for all streamable types
	   /*	template <class T>
        element& contents(T value) {
            std::stringstream ss;
            ss << value;
            contents(ss.str());
			return *this;
		}  */

        // write CDATA section
        element& cdata(const char* str) {
		  //  assert(str != 0);
            check_parent();
            wr.xputs("<![CDATA[");
            wr.sxputs(str);
            wr.xputs("]]>");
            return *this;
        }

        // cdata() overload for std::string type
        element& cdata(const std::string& str) { return cdata(str.c_str()); }

    private:
        writer& wr;         // bound XML writer
        const char* name;   // name of current element
        bool tagopen;       // is the element tag for this element still open?

        // write a string quoting characters which have meaning in xml
        element& qxputs(const char* str) {
            for (; *str; ++str)
                switch (*str) {
                    case '&': wr.xputs("&amp;"); break;
                    case '<': wr.xputs("&lt;"); break;
                    case '>': wr.xputs("&gt;"); break;
                    case '\'': wr.xputs("&apos;"); break;
                    case '"': wr.xputs("&quot;"); break;
                    default: wr.xputc(*str); break;
                }
            return *this;
        }

        // check to see if we have a parent tag which needs to be closed
        void check_parent() {
            if (!wr.elements.empty() && wr.elements.top()->tagopen) {
                wr.xputc('>');
                wr.elements.top()->tagopen = false;
            }
        }
    };

}

#endif
