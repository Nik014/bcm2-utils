#ifndef BCM2DUMP_DUMPER_H
#define BCM2DUMP_DUMPER_H
#include <memory>
#include <string>
#include "interface.h"
#include "profile.h"

namespace bcm2dump {
class reader : public reader_writer
{
	public:
	typedef std::shared_ptr<reader> sp;

	virtual ~reader() {}

	virtual uint32_t offset_alignment() const
	{ return 4; }

	virtual uint32_t length_alignment() const
	{ return 16; }

	virtual uint32_t chunk_size() const = 0;

	virtual void dump(const addrspace::part& partition, std::ostream& os, uint32_t length = 0);
	virtual void dump(uint32_t offset, uint32_t length, std::ostream& os);
	std::string read(uint32_t offset, uint32_t length);

	static sp create(const interface::sp& interface, const std::string& type, bool no_dumpcode = false);

	protected:
	virtual std::string read_chunk(uint32_t offset, uint32_t length) = 0;
};
}
#endif