#ifndef __UNN_FLAGS_H_
#define __UNN_FLAGS_H_

// f can't be 0

#define flag_toggle(flags, f) (flags = flags ^ f)

#define flag_on(flags, f) (flags = flags | f)

#define flag_off(flags, f) (flags = flags ^ (flags & f))

#define flag_is_on(flags, f) (flags & f)

#define flag_is_off(flags, f) (flags ^ f)

#endif