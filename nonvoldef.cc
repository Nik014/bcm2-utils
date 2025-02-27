/**
 * bcm2-utils
 * Copyright (C) 2016 Joseph Lehner <joseph.c.lehner@gmail.com>
 *
 * bcm2-utils is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bcm2-utils is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bcm2-utils.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "nonvoldef.h"
#include "util.h"
using namespace std;

namespace bcm2cfg {
namespace {

template<size_t N> class nv_cdata : public nv_data
{
	public:
	nv_cdata() : nv_data(N) {}
};

template<class T> const sp<T>& nv_val_disable(const sp<T>& val, bool disable)
{
	val->disable(disable);
	return val;
}

template<class T> const sp<T>& nv_compound_rename(const sp<T>& val, const std::string& name)
{
	val->rename(name);
	return val;
}

bool is_zero_mac(const csp<nv_mac>& mac)
{
	return mac->to_str() == "00:00:00:00:00:00";
}

bool is_empty_string(const csp<nv_string>& str)
{
	return str->to_str().empty();
}

class nv_timestamp : public nv_u32
{
	public:
	virtual string to_string(unsigned, bool) const override
	{
		char buf[128];
		time_t time = num();
		strftime(buf, sizeof(buf) - 1, "%F %R", localtime(&time));
		return buf;
	}
};

class nv_time_period : public nv_compound_def
{
	public:
	nv_time_period()
	: nv_compound_def("time-period", {
			NV_VAR(nv_u8_m<23>, "beg_hrs"),
			NV_VAR(nv_u8_m<23>, "end_hrs"),
			NV_VAR(nv_u8_m<59>, "beg_min"),
			NV_VAR(nv_u8_m<59>, "end_min"),
	}) {}

	string to_string(unsigned level, bool pretty) const override
	{
		if (!pretty) {
			return nv_compound_def::to_string(level, pretty);
		}

		return num("beg_hrs") + ":" + num("beg_min") + "-" +
				num("end_hrs") + ":" + num("end_min");
	}

	private:
	string num(const string& name) const
	{
		string str = get(name)->to_pretty();
		return (str.size() == 2 ? "" : "0") + str;
	}
};

class nv_ipstacks : public nv_bitmask<nv_u8>
{
	public:
	nv_ipstacks() : nv_bitmask("ipstacks", names())
	{}

	static valvec names()
	{
		return { "IP1", "IP2", "IP3", "IP4", "IP5", "IP6", "IP7", "IP8" };
	}
};

class nv_ipstack : public nv_enum<nv_u8>
{
	public:
	nv_ipstack() : nv_enum("ipstack", nv_ipstacks::names())
	{}
};

class nv_annex_mode : public nv_enum<nv_u8>
{
	public:
	nv_annex_mode() : nv_enum("annex_mode", valvec { "B", "A", "J", "other", "C" })
	{}
};

class nv_group_mlog : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_mlog, "MLog", "userif");

	protected:

	class nv_remote_acc_methods : public nv_bitmask<nv_u8>
	{
		public:
		nv_remote_acc_methods() : nv_bitmask("remote_acc_methods", {
				"telnet", "http", "ssh"
		}) {}
	};

	virtual list definition(int type, const nv_version& ver) const override
	{
		if (type == fmt_perm) {
			return {
				NV_VAR2(nv_bitmask<nv_u32>, "log_severities", {
						"fatal",
						"error",
						"warning",
						"fn_entry_exit",
						"trace",
						"info",
				}),
				NV_VAR2(nv_bitmask<nv_u8>, "log_fields", {
						"severity",
						"instance",
						"function",
						"module",
						"timestamp",
						"thread",
						"milliseconds"
				}),
#if 1
				NV_VAR(nv_remote_acc_methods, "remote_acc_methods"),
				NV_VAR(nv_cdata<16>, "remote_acc_user"),
				NV_VAR(nv_cdata<16>, "remote_acc_pass"),
#else
				NV_VAR(nv_cdata<33>, ""),
#endif
				NV_VAR(nv_ip4, "remote_acc_ip"),
				NV_VAR(nv_ip4, "remote_acc_subnet"),
				NV_VAR(nv_ip4, "remote_acc_router"),
				NV_VAR(nv_u8, "remote_acc_ipstack"),
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_u8, "virtual_enet_ipstack"),
			};
		}

		auto flags = profile() ? profile()->cfg_flags() : 0;

		if (flags & BCM2_CFG_DATA_USERIF_ALT || flags & BCM2_CFG_DATA_USERIF_ALT_SHORT) {
			return {
				NV_VAR(nv_p16string, "http_user", 32),
				NV_VAR(nv_p16string, "http_pass", 32),
				NV_VAR(nv_p16string, "http_admin_user", 32),
				NV_VAR(nv_p16string, "http_admin_pass", 32),
				NV_VAR(nv_p16string, "http_local_user", 16),
				NV_VAR(nv_p16string, "http_local_pass", 16),
				NV_VAR(nv_p16string, "http_default_pass"),
				NV_VAR(nv_p16string, "http_erouter_user"),
				NV_VAR(nv_p16string, "http_erouter_pass"),
				NV_VAR2(nv_remote_acc_methods, "remote_acc_methods"),
				NV_VAR(nv_fzstring<16>, "remote_acc_user"),
				NV_VAR3(flags & BCM2_CFG_DATA_USERIF_ALT, nv_data, "", 112),
				NV_VAR2(nv_ipstacks, "telnet_ipstacks"),
				NV_VAR2(nv_ipstacks, "ssh_ipstacks"),
				NV_VAR2(nv_u32, "remote_acc_timeout"),
				NV_VAR2(nv_ipstacks, "http_ipstacks"),
				NV_VAR2(nv_ipstacks, "http_adv_ipstacks"),
				NV_VAR2(nv_p16string, "http_seed"),
				NV_VAR2(nv_p16data, "http_acl_hosts"),
				NV_VAR2(nv_u32, "http_idle_timeout"),
				NV_VAR2(nv_bool, "log_exceptions"),
			};
		} else {
			return {
				NV_VAR(nv_p16string, "http_user", 32),
				NV_VAR(nv_p16string, "http_pass", 32),
				NV_VAR(nv_p16string, "http_admin_user", 32),
				NV_VAR(nv_p16string, "http_admin_pass", 32),
				NV_VAR(nv_remote_acc_methods, "remote_acc_methods"),
				NV_VAR(nv_fzstring<16>, "remote_acc_user"),
				NV_VAR(nv_fzstring<16>, "remote_acc_pass"),
				NV_VAR(nv_ipstacks, "telnet_ipstacks"),
				NV_VAR(nv_ipstacks, "ssh_ipstacks"),
				NV_VAR(nv_u32, "remote_acc_timeout"),
				NV_VAR(nv_ipstacks, "http_ipstacks"),
				NV_VAR(nv_ipstacks, "http_adv_ipstacks"),
				NV_VAR(nv_p16string, "http_seed"),
				NV_VAR(nv_p16data, "http_acl_hosts"),
				NV_VAR(nv_u32, "http_idle_timeout"),
#if 0
				NV_VAR3(ver.num() >= 0x0006, nv_bool, "log_exceptions"),
				NV_VAR(nv_u32, "ssh_inactivity_timeout"),
#endif
			};
		}
	}
};

class nv_group_cmap : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_cmap, "CMAp", "bfc")

	protected:
	virtual list definition(int type, const nv_version& ver) const override
	{
		if (type == fmt_perm) {
			return {
				NV_VAR2(nv_bitmask<nv_u8>, "console_features", {
						"stop_at_console",
						"skip_driver_init_prompt",
						"stop_at_console_prompt"
				}),
			};
		} else {
			return {
				NV_VAR2(nv_enum<nv_u32>, "serial_console_mode", "", {
						"disabled", "ro", "rw", "factory"
				}),
				NV_VAR2(nv_bitmask<nv_u32>, "features", "", {
						"aux_serial_console",
				}),
			};
		};
	}
};

class nv_group_snmp : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_snmp, "snmp", "snmp")

	protected:
	virtual list definition(int type, const nv_version& ver) const override
	{
		if (type == fmt_perm) {
			return {
				NV_VAR(nv_bool, "allow_config"),
				NV_VAR(nv_cdata<0x20>, ""),
				NV_VAR(nv_cdata<0x80>, ""),
				NV_VAR(nv_cdata<0x80>, ""),
				NV_VAR(nv_fzstring<0x80>, "sys_contact"),
				NV_VAR(nv_fzstring<0x80>, "sys_name"),
				NV_VAR(nv_fzstring<0x80>, "sys_location"),
				NV_VAR(nv_cdata<0x80>, ""),
				NV_VAR(nv_cdata<0x80>, ""),
				NV_VAR(nv_u8, ""),
				NV_VAR(nv_cdata<0x80>, ""),
				NV_VAR(nv_fzstring<0x40>, "serial_number"),
				// version 0.2
				NV_VAR(nv_u8, "max_download_tries"),
				// version 0.3
				NV_VAR(nv_cdata<4>, ""),
				// version 0.4
				NV_VAR(nv_cdata<0x80>, ""),
			};
		} else {
			return {
				NV_VAR2(nv_enum<nv_u8>, "docs_dev_sw_admin_status", "docs_dev_sw_admin_status", {
						"unknown",
						"upgrade_from_mgmt",
						"allow_provisioning_upgrade",
						"ignore_provisioning_upgrade"
				}),
				NV_VAR2(nv_enum<nv_u8>, "docs_dev_sw_oper_status", "docs_dev_sw_oper_status", {
						"unknown",
						"in_progress",
						"complete_from_provisioning",
						"complete_from_mgmt"
						"failed"
						"other"
				}),
				NV_VAR(nv_fzstring<0x100>, "docs_dev_sw_file_name"),
				NV_VAR(nv_ip4, "docs_dev_sw_server"),
				NV_VAR3(ver.num() < 0x0002, nv_u16, ""),
				// version 0.2
				NV_VAR(nv_p16list<nv_cdata<12>>, "_list1"), // this list's size is reused later
				NV_VAR(nv_p16list<nv_cdata<4 + 2 + 4 + 4>>, ""),
				// version 0.3
				NV_VAR(nv_fzstring<0x100>, "sys_contact"),
				NV_VAR(nv_fzstring<0x100>, "sys_name"),
				NV_VAR(nv_fzstring<0x100>, "sys_location"),
				// version 0.4
				NV_VAR(nv_u8, "num_download_tries"),
				// version 0.5
				NV_VAR(nv_u32, "num_engine_boots"),
#if 0
				// version 0.6
				// since we don't yet support size specifiers other than 
				// prefixes, we can't continue for now. what follows is an
				// array of nv_p16data objects, whose size is the same as
				// the one of the first nv_p16list of version 0.2 (see above).
				NV_VAR(nv_pxlist<nv_p16data>, "", "_list1.elements"),
#endif
			};
		}
	}
};

class nv_group_thom : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_thom, "THOM", "thombfc")

	protected:
	class nv_eth_ports : public nv_bitmask<nv_u32>
	{
		public:
		nv_eth_ports()
		: nv_bitmask<nv_u32>("eth-ports", {
			{ 0x02000, "eth1" },
			{ 0x04000, "eth2" },
			{ 0x08000, "eth3" },
			{ 0x10000, "eth4" },
		}) {}
	};

	virtual list definition(int type, const nv_version& ver) const override
	{
		return {
			// 0x6 = rw (0x4 = write, 0x2 = read)
			// 0x6 = rw (0x4 = rw, 0x2 = enable (?), 0x1 = factory (?))
			NV_VAR2(nv_bitmask<nv_u8>, "serial_console_mode", {
				"", "read", "write", "factory"
			}),
#if 0
			NV_VAR(nv_bool, "early_console_enable"), // ?
			NV_VAR(nv_u16, "early_console_bufsize", true),
			NV_VAR(nv_u8, "", true), // maybe this is early_console_enable?
			NV_VAR(nv_data, "", 3),
			// 1 = lan_access
			NV_VAR(nv_bitmask<nv_u8>, "features"),
			NV_VAR(nv_data, "", 4),
			NV_VAR(nv_eth_ports, "pt_interfacemask"),
			NV_VAR(nv_eth_ports, "pt_interfaces"),
#endif
		};
	}

};

class nv_group_8021 : public nv_group
{
	public:
	nv_group_8021(bool card2)
	: nv_group(card2 ? "8022" : "8021", "bcmwifi"s + (card2 ? "2" : ""))
	{}

	NV_GROUP_DEF_CLONE(nv_group_8021);

	class nv_wifi_encmode : public nv_enum<nv_u8>
	{
		public:
		nv_wifi_encmode() : nv_enum<nv_u8>("encryption", {
				"none", "wep64", "wep128", "tkip", "aes", "tkip_aes",
				"tkip_wep64", "aes_wep64", "tkip_aes_wep64", "tkip_wep128",
				"aes_wep128", "tkip_aes_wep128"
		})
		{}
	};

	class nv_wpa_auth : public nv_bitmask<nv_u8>
	{
		public:
		nv_wpa_auth() : nv_bitmask<nv_u8>("wpa-auth",
				{ "802.1x", "wpa1", "psk1", "wpa2", "psk2"
		})
		{}
	};

	protected:

	class nv_wmm : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_wmm, "wmm");

		protected:
		class nv_wmm_block : public nv_compound
		{
			public:
			NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_wmm_block, "wmm-block");

			protected:
			class nv_wmm_params : public nv_compound
			{
				public:
				NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_wmm_params, "wmm-params");

				protected:
				virtual list definition() const override
				{
					typedef nv_u16_r<0, 15> cwminaifs;
					typedef nv_u16_r<0, 1024> cwmax;
					typedef nv_u16_r<0, 8192> txop;

					return {
						NV_VAR(cwminaifs, "cwmin"),
						NV_VAR(cwmax, "cwmax"),
						NV_VAR(cwminaifs, "aifsn"),
						NV_VAR(txop, "txop_b"),
						NV_VAR(txop, "txop_ag")
					};
				}
			};

			virtual list definition() const override
			{
				return {
					NV_VARN(nv_wmm_params, "sta"),
					NV_VARN(nv_wmm_params, "ap"),
					NV_VAR(nv_bool, "ap_adm_control"),
					NV_VAR(nv_bool, "ap_oldest_first"),
				};
			}
		};

		virtual list definition() const override
		{
			return {
				NV_VARN(nv_wmm_block, "ac_be"),
				NV_VARN(nv_wmm_block, "ac_bk"),
				NV_VARN(nv_wmm_block, "ac_vi"),
				NV_VARN(nv_wmm_block, "ac_vo"),
			};
		}

	};

	virtual list definition(int type, const nv_version& ver) const override
	{
		if (type == fmt_perm) {
			return nv_group::definition(type, ver);
		}

		// known versions
		// 0x0015: TWG850
		// 0x001d: TWG870
		// 0x0021: TCW770
		// 0x0024: TC7200
		//
		// in its current form, only 0x0015 needs special care, whereas
		// 0x001d-0x0024 appear to be the same, at least until
		// in_network_radar_check

		typedef nv_u16_r<20, 1024> beacon_interval;
		typedef nv_u16_r<1, 255> dtim_interval;
		typedef nv_u16_r<256, 2346> frag_threshold;
		typedef nv_u16_r<1, 2347> rts_threshold;

		// XXX quite possibly an nv_u16
		class nv_wifi_rate_mbps : public nv_enum<nv_u8>
		{
			public:
			nv_wifi_rate_mbps() : nv_enum<nv_u8>("rate-mbps", nv_enum<nv_u8>::valmap {
				{ 0x00, "auto" },
				{ 0x02, "1" },
				{ 0x04, "2" },
				{ 0x0b, "5.5" },
				{ 0x0c, "6" },
				{ 0x12, "9" },
				{ 0x16, "11" },
				{ 0x18, "12" },
				{ 0x24, "18" },
				{ 0x30, "24" },
				{ 0x48, "36" },
				{ 0x60, "48" },
				{ 0x6c, "54" }
			})
			{}
		};

		const nv_enum<nv_u8>::valvec offauto = { "off", "auto" };

		if (type != fmt_perm) {
			return {
				NV_VAR(nv_zstring, "ssid", 33),
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_u8, "b_channel"),
				NV_VAR(nv_u8, "", true),
				// 0x0f = all
				NV_VAR(nv_u8, "basic_rates", true), // XXX u16?
				NV_VAR(nv_data, "", 1),
				NV_VAR(nv_u8, "supported_rates", true), // 0x03 = min, 0x0f = all (1..11Mbps)
				NV_VAR(nv_data, "", 1),
				NV_VAR(nv_wifi_encmode, "encryption"),
				NV_VAR(nv_data, "", 1),
				NV_VAR(nv_u8_r<1 COMMA() 3>, "authentication"), // 1 = open, 2 = shared key, 3 = both
				NV_VAR(nv_array<nv_cdata<5>>, "wep64_keys", 4),
				NV_VAR(nv_u8, "wep_key_num"),
				NV_VAR(nv_cdata<13>, "wep128_key_1"),
				NV_VAR(beacon_interval, "beacon_interval"),
				NV_VAR(dtim_interval, "dtim_interval"),
				NV_VAR(frag_threshold, "frag_threshold"),
				NV_VAR(rts_threshold, "rts_threshold"),
				NV_VAR(nv_array<nv_cdata<13>>, "wep128_keys", 3),
				NV_VAR2(nv_enum<nv_u8>, "mac_policy", "mac_policy", { "disabled", "allow", "deny" }),
				NV_VARN(nv_array<nv_mac>, "mac_table", 32, &is_zero_mac),
				NV_VAR(nv_bool, "preamble_long"),
				NV_VAR(nv_bool, "hide_ssid"),
				NV_VAR(nv_u8_r<1 COMMA() 8>, "txpower_level"),
				NV_VAR(nv_data, "", 0x20),
				NV_VAR(nv_u8, "short_retry_limit"),
				NV_VAR(nv_u8, "long_retry_limit"),
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_u8, "a_channel"),
				// 1 = auto, 4 = performance, 5 = lrs
				NV_VAR2(nv_enum<nv_u8>, "g_mode", "g_mode", { "b", "auto", "g", "", "", "performance", "lrs" }),
				NV_VAR(nv_bool, "radio_enabled"),
				NV_VAR(nv_bool, "g_protection"),
				NV_VAR(nv_data, "", 1),
				NV_VAR(nv_wifi_rate_mbps, "g_rate"),
				NV_VAR(nv_u8_m<100>, "tx_power"),
				NV_VAR(nv_p16string, "wpa_psk"),
				NV_VAR(nv_data, "", 0x2),
				// wpa_rekey
				NV_VAR(nv_u16, "wpa_rekey_interval"),
				NV_VAR(nv_ip4, "radius_ip"),
				NV_VAR(nv_u16, "radius_port"),
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_p8string, "radius_key"),
				// [size-5]: nv_bool frame_burst_enabled
				// [size-6]: nv_bool shared_key_auth_required
				NV_VAR(nv_data, "", (ver.num() <= 0x0015 ? 0x56 : 0x2a) - 0x1d),
				NV_VAR(nv_bool, "wds_enabled"),
				NV_VAR(nv_array<nv_mac>, "wds_list", 4),
				NV_VAR(nv_bool, "enable_afterburner"),
				NV_VAR(nv_data, "", 3),
				NV_VAR2(nv_bitmask<nv_u8>, "wpa_auth", "wpa_auth", { "802.1x", "wpa1", "psk1", "wpa2", "psk2" }),
				NV_VAR(nv_data, "", 2),
				NV_VAR(nv_u16, "wpa_reauth_interval"),
				NV_VAR(nv_bool, "wpa2_preauth_enabled"),
				NV_VAR(nv_data, "", 3),
				NV_VAR(nv_bool, "wmm_enabled"),
				NV_VAR(nv_bool, "wmm_nak"),
				NV_VAR(nv_bool, "wmm_powersave"),
				// nv_i32(?) wmm_vlan_mode: -1 = auto, 0 = off, 1 = on
				NV_VAR(nv_data, "", 4),
				NV_VARN(nv_wmm, "wmm"),
				NV_VAR2(nv_enum<nv_u8>, "n_band", "band", { "", "2.4Ghz", "5Ghz" }),
				NV_VAR(nv_u8, "n_control_channel"),
				NV_VAR2(nv_enum<nv_u8>, "n_mode", "off_auto", offauto), // 0 = off, 1 = auto
				NV_VAR2(nv_enum<nv_u8>, "n_bandwidth", "n_bandwidth", nv_enum<nv_u8>::valmap {
					{ 10, "10MHz" },
					{ 20, "20MHz" },
					{ 40, "40MHz" }
				}),
				NV_VAR2(nv_enum<nv_i8>, "n_sideband", "sideband", nv_enum<nv_i8>::valmap {
					{ -1, "lower" },
					{ 0, "none" },
					{ 1, "upper"}
				}),
				NV_VAR2(nv_enum<nv_i8>, "n_rate", "n_rate", nv_enum<nv_i8>::valmap {
						{ -2, "legacy" },
						{ -1, "auto" },
						{ 0, "0" },
						{ 1, "1" },
						{ 2, "2" },
						{ 3, "3" },
						{ 4, "4" },
						{ 5, "5" },
						{ 6, "6" },
						{ 7, "7" },
						{ 8, "8" },
						{ 9, "9" },
						{ 10, "10" },
						{ 11, "11" },
						{ 12, "12" },
						{ 13, "13" },
						{ 14, "14" },
						{ 15, "15" },
						{ 32, "mcs_index" },
				}),
				NV_VAR2(nv_enum<nv_u8>, "n_protection", "off_auto", offauto), // 0 = off, 1 = auto
				NV_VAR(nv_bool, "wps_enabled"),
				NV_VAR(nv_bool, "wps_configured"),
				// these appear to be p8string on some devices
				NV_VAR(nv_p8string, "wps_device_pin"),
				NV_VAR(nv_p8zstring, "wps_model"),
				NV_VAR(nv_p8zstring, "wps_manufacturer"),
				NV_VAR(nv_p8zstring, "wps_device_name"),
				// ?? nv_bool wps_external ??
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_p8zstring, "wps_sta_pin"),
				NV_VAR(nv_p8zstring, "wps_model_num"),
				NV_VAR(nv_bool, "wps_timeout"),
				NV_VAR(nv_data, "", 1),
				NV_VAR(nv_p8zstring, "wps_uuid"),
				NV_VAR(nv_p8zstring, "wps_board_num"),
				// ?? stays 0 when setting to "configed" (?wps_sta_mac_restrict?)
				NV_VAR(nv_bool, "wps_configured2"),
				NV_VAR(nv_p8zstring, "country"),
				NV_VAR(nv_bool, "primary_network_enabled"),
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_wifi_rate_mbps, "a_mcast_rate"),
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_wifi_rate_mbps, "bg_mcast_rate"),
				NV_VAR2(nv_enum<nv_u8>, "reg_mode", "reg_mode", {
					"off", "802.11d", "802.11h"
				}),
				NV_VAR(nv_u8_m<99>, "pre_network_radar_check"),
				NV_VAR(nv_u8_m<99>, "in_network_radar_check"),
				// XXX enum: 0 (off), 2, 3, 4 dB
				NV_VAR2(nv_enum<nv_u8>, "tpc_mitigation", "", {
						"off", "", "2dB", "3dB", "4dB"
				}),
				NV_VAR2(nv_bitmask<nv_u8>, "features", nv_bitmask<nv_u8>::valmap {
					{ 0x01, "obss_coex" },
					{ 0x04, "ap_isolate" },
					{ 0x10, "stbc_tx_on" },
					{ 0x20, "stbc_tx_off" },
					{ 0x40, "sgi_on" },
					{ 0x80, "sgi_off" },
				}),
				NV_VAR(nv_data, "", 1),
				// 0 = default, 1 = legacy, 2 = intf, 3 = intf-busy, 4 = optimized,
				// 5 = optimized1, 6 = optimized2, 7 = user
				NV_VAR(nv_u8_m<7>, "acs_policy_index"),
				NV_VAR(nv_data, "", 3),
				NV_VAR(nv_u8, "acs_flags"),
				NV_VAR(nv_p8string, "acs_user_policy"),
				NV_VAR(nv_p8list<nv_u32>, ""),
				NV_VAR(nv_u8_m<128>, "bss_max_assoc"),
				NV_VAR2(nv_enum<nv_u8>, "bandwidth_cap", "", nv_enum<nv_u8>::valmap {
					{ 0x01, "20MHz" },
					{ 0x03, "40MHz" },
					{ 0x07, "80MHz" }
				}),
				// this is the same as n_control_channel, but as text (could also be a p8zstring)
				NV_VAR(nv_p8istring, ""),
				NV_VAR(nv_data, "", 0x13),
				// 0x01 = txchain(1), 0x03 = txchain(2), 0x07 = txchain(3)
				NV_VAR(nv_u8, "txchain", true),
				// same as above
				NV_VAR(nv_u8, "rxchain", true),
				NV_VAR2(nv_enum<nv_u8>, "mimo_preamble", "", {
						"mixed", "greenfield", "greenfield_broadcom"
				}),
				NV_VAR2(nv_bitmask<nv_u8>, "features2", nv_bitmask<nv_u8>::valvec {
						"ampdu", "amsdu",
				}),
				NV_VAR(nv_u32, "acs_cs_scan_timer"),
				NV_VAR(nv_u8_m<3>, "bss_mode_reqd", true),
				NV_VAR(nv_data, "", 3),
				NV_VAR2(nv_bitmask<nv_u8>, "features3", nv_bitmask<nv_u8>::valmap {
					{ 0x01, "band_steering" },
					{ 0x02, "airtime_fairness" },
					{ 0x04, "traffic_scheduler" },
					{ 0x08, "exhausted_buf_order_sched" },
				}),
			};
		}

		return nv_group::definition(type, ver);
	}
};

class nv_group_t802 : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_t802, "T802", "tmmwifi")

	protected:
	virtual list definition(int type, const nv_version& ver) const override
	{
		if (type == fmt_perm) {
			return nv_group::definition(type, ver);
		}

		return {
#if 0
			NV_VAR(nv_array<nv_u8 COMMA() 10>, "wifi_sleep"),
#else
			NV_VAR(nv_bool, "sleep_breaking_time"),
			NV_VAR(nv_bool, "sleep_every_day"),
			NV_VAR(nv_bitmask<nv_u8>, "sleep_days"),
			NV_VAR(nv_bool, "sleep_all_day"),
#if 0
			NV_VAR(nv_u8_m<23>, "sleep_begin_h"),
			NV_VAR(nv_u8_m<23>, "sleep_end_h"),
			NV_VAR(nv_u8_m<59>, "sleep_begin_m"),
			NV_VAR(nv_u8_m<59>, "sleep_end_m"),
#else
			NV_VAR(nv_time_period, "sleep_time"),
#endif
			NV_VAR(nv_bool, "sleep_enabled"),
			NV_VAR(nv_bool, "sleep_page_visible"),
#endif
			NV_VAR(nv_data, "", 4),
			NV_VAR(nv_fzstring<33>, "ssid_24"),
			NV_VAR(nv_fzstring<33>, "ssid_50"),
			NV_VAR(nv_u8, "", true),
			NV_VAR(nv_p8string, "wpa_psk_24"),
			NV_VAR(nv_u8, "", true),
			NV_VAR(nv_p8string, "wpa_psk_50"),
			NV_VAR(nv_data, "", 4),
			NV_VAR(nv_fzstring<33>, "wifi_opt60_replace"),
			NV_VAR(nv_data, "", 8),
			NV_VAR(nv_fstring<33>, "card1_prefix"),
			// the firmware refers to this as "Card-1 Ramdon"
			NV_VAR(nv_fzstring<33>, "card1_random"),
			NV_VAR(nv_fzstring<33>, "card2_prefix"),
			NV_VAR(nv_fzstring<33>, "card2_random"),
			NV_VAR(nv_u8_m<99>, "card1_regul_rev"),
			NV_VAR(nv_u8_m<99>, "card2_regul_rev"),
		};
	}
};

class nv_group_rg : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_rg, "RG..", "rg")

	protected:
	template<int N> class nv_ip_range : public nv_compound
	{
		public:
		nv_ip_range() : nv_compound(false) {}

		virtual string type() const override
		{ return "ip" + ::to_string(N) + "_range"; }

		virtual string to_string(unsigned level, bool pretty) const override
		{
			return get("start")->to_string(level, pretty) + "," + get("end")->to_string(level, pretty);
		}


		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_ip<N>, "start"),
				NV_VAR(nv_ip<N>, "end")
			};
		}
	};

	class nv_ip4_range : public nv_ip_range<4>
	{
		public:
		static bool is_end(const csp<nv_ip4_range>& r)
		{
			return r->get("start")->to_str() == "0.0.0.0" && r->get("end")->to_str() == "0.0.0.0";
		}
	};

	class nv_port_range : public nv_compound
	{
		public:
		nv_port_range() : nv_compound(false) {}

		virtual string type() const override
		{ return "port-range"; }

		static bool is_range(const csp<nv_port_range>& r, uint16_t start, uint16_t end)
		{
			return r->get_as<nv_u16>("start")->num() == start && r->get_as<nv_u16>("end")->num() == end;
		}

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_u16, "start"),
				NV_VAR(nv_u16, "end")
			};
		}

	};

	class nv_proto : public nv_enum<nv_u8>
	{
		public:
		// FIXME in RG 0x0016, TCP and UDP are reversed!
		nv_proto(bool reversed = false) : nv_enum<nv_u8>("protocol", {
			{ 0x3, reversed ? "UDP" : "TCP" },
			{ 0x4, reversed ? "TCP" : "UDP" },
			{ 0xfe, "TCP+UDP" }
		}) {}
	};

	class nv_port_forward : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_port_forward, "port-forward");

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_ip4, "dest"),
				NV_VAR(nv_port_range, "ports"),
				NV_VAR(nv_proto, "type"),
			};
		}
	};

	class nv_port_forward_dport : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_port_forward_dport, "port-forward-dport");

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_port_range, "ports"),
				NV_VAR(nv_data, "data", 4),
			};
		}
	};

	class nv_port_trigger : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_port_trigger, "port-trigger");

		static bool is_end(const csp<nv_port_trigger>& pt)
		{
			return nv_port_range::is_range(pt->get_as<nv_port_range>("trigger"), 0, 0);
		}

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_port_range, "trigger"),
				NV_VAR(nv_port_range, "target"),
			};
		}
	};

	template<bool ROUTE1> class nv_route : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_route, "route");

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_ip4, ROUTE1 ? "netmask" : "network"),
				NV_VAR(nv_ip4, ROUTE1 ? "network" : "gateway"),
				NV_VAR(nv_ip4, ROUTE1 ? "gateway" : "netmask"),
			};
		}
	};

	virtual list definition(int type, const nv_version& ver) const override
	{
		typedef nv_i32_r<-45000, 45000> timezone_offset;

		// TWG870: version 0x0016 (0.22)

		return {
			NV_VAR(nv_bool, "router_mode"),
			NV_VAR(nv_zstring, "http_pass", 9),
			NV_VAR(nv_zstring, "http_realm", 256),
			NV_VAR(nv_mac, "spoofed_mac"),
			NV_VAR2(nv_bitmask<nv_u32>, "features1", nv_bitmask<nv_u32>::valvec {
				"wan_conn_pppoe",
				"", // 0x02 (unset by default, automatically removed if set)
				"feature_ip_filters",
				"feature_port_filters",
				"wan_block_pings",
				"feature_ipsec_passthrough",
				"feature_pptp_passthrough",
				"wan_remote_cfg_mgmt",
				"feature_ip_forwarding", // 0x0100 (unset by default)
				"feature_dmz",
				"wan_conn_static",
				"feature_nat_debug",
				"lan_dhcp_server",
				"lan_http_server",
				"primary_default_override",
				"feature_mac_filters",
				"feature_port_triggers",
				"feature_multicast",
				"wan_rip",
				"", // 0x080000 (unset by default)
				"feature_dmz_by_hostname",
				"lan_upnp",
				"lan_routed_subnet",
				"lan_routed_subnet_dhcp",
				"wan_passthrough_skip_dhcp",
				"lan_routed_subnet_nat",
				"", // 0x04000000
				"wan_sntp",
				"wan_conn_pptp",
				"wan_pptp_server",
				"feature_ddns",
				"", // 0x80000000
			}),
			NV_VAR(nv_ip4, "dmz_ip"),
			NV_VAR(nv_ip4, "wan_ip"),
			NV_VAR(nv_ip4, "wan_mask"),
			NV_VAR(nv_ip4, "wan_gateway"),
			NV_VAR(nv_fzstring<0x100>, "wan_dhcp_hostname"),
			NV_VAR(nv_fzstring<0x100>, "syslog_email"),
			NV_VAR(nv_fzstring<0x100>, "syslog_smtp"),
			NV_VARN(nv_array<nv_ip4_range>, "ip_filters", 10 , &nv_ip4_range::is_end),
			NV_VARN(nv_array<nv_port_range>, "port_filters", 10, [] (const csp<nv_port_range>& range) {
				return nv_port_range::is_range(range, 1, 0xffff);
			}),
			NV_VARN(nv_array<nv_port_forward>, "port_forwards", 10, [] (const csp<nv_port_forward>& fwd) {
				return fwd->get("dest")->to_str() == "0.0.0.0";
			}),
			NV_VARN(nv_array<nv_mac>, "mac_filters", 20, &is_zero_mac),
			NV_VARN(nv_array<nv_port_trigger>, "port_triggers", 10, &nv_port_trigger::is_end),
			NV_VAR(nv_data, "", 0x15),
			NV_VARN(nv_array<nv_proto>, "port_filter_protocols", 10),
			NV_VAR(nv_data, "", 0xaa),
			NV_VARN(nv_array<nv_proto>, "port_trigger_protocols", 10),
			NV_VAR(nv_data, "", 0x443),
			NV_VAR(nv_data, "", 3),
			NV_VAR(nv_p8string, "rip_key"),
			NV_VAR(nv_u16, "rip_reporting_interval"),
			NV_VAR(nv_data, "", 0xa),
			NV_VAR(nv_route<true>, "route1"),
			NV_VAR(nv_route<false>, "route2"),
			NV_VAR(nv_route<false>, "route3"),
			NV_VAR(nv_ip4, "nat_route_gateway"),
			NV_VAR(nv_array<nv_ip4>, "nat_route_dns", 3),
			NV_VAR(nv_p8string, "l2tp_username"),
			NV_VAR(nv_p8string, "l2tp_password"),
			NV_VAR(nv_data, "", 5),
			NV_VARN(nv_p8list<nv_p8string>, "timeservers"),
			NV_VAR(timezone_offset, "timezone_offset"),
			NV_VARN3(ver.num() > 0x0016, nv_array<nv_port_forward_dport>, "port_forward_dports", 10, [] (const csp<nv_port_forward_dport>& range) {
				return nv_port_range::is_range(range->get_as<nv_port_range>("ports"), 0, 0);
			}),
			NV_VAR(nv_p16string, "ddns_username"),
			NV_VAR(nv_p16string, "ddns_password"),
			NV_VAR(nv_p16string, "ddns_hostname"),
			NV_VAR(nv_data, "", 4),
			NV_VAR(nv_u16, "mtu"),
			NV_VAR(nv_data, "", 3),
			// 0x01 = wan_l2tp_server , 0x02 = wan_conn_l2tp (?)
			NV_VAR2(nv_bitmask<nv_u8>, "features2", nv_bitmask<nv_u8>::valvec {
				"wan_l2tp_server",
				"wan_conn_l2tp"
			}),
			NV_VAR(nv_ip4, "l2tp_server_ip"),
			NV_VAR(nv_p8string, "l2tp_server_name"), // could also be a p8zstring

		};
	}
};

class nv_group_cdp : public nv_group
{
	public:

	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_cdp, "CDP.", "dhcp")

	protected:
	class nv_ip4_typed : public nv_compound
	{
		public:
		nv_ip4_typed() : nv_compound(false) {}

		virtual string type() const override
		{ return "typed_ip"; }

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_u32, "type"),
				NV_VAR(nv_ip4, "ip")
			};
		}
	};

	class nv_lan_addr_entry : public nv_compound
	{
		public:
		nv_lan_addr_entry() : nv_compound(false) {}

		string type() const override
		{ return "lan_addr"; }

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_u16, "num_1"),
				NV_VAR(nv_u16, "create_time"),
				NV_VAR(nv_u16, "num_2"),
				NV_VAR(nv_u16, "expire_time"),
				NV_VAR(nv_u8, "ip_type"),
				NV_VAR(nv_ip4, "ip"),
				NV_VAR(nv_data, "ip_data", 3),
				NV_VAR(nv_u8, "method"),
				NV_VAR(nv_p8data, "client_id"),
				NV_VAR(nv_p8string, "hostname"),
				NV_VAR(nv_mac, "mac"),
			};
		}
	};

	class nv_wan_dns_entry : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_wan_dns_entry, "wan-dns-entry")

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_ip4, "ip")
			};
		}
	};

	virtual list definition(int type, const nv_version& ver) const override
	{
		return {
			NV_VAR(nv_data, "", 7),
			NV_VAR(nv_u8, "lan_trans_threshold"),
			NV_VAR(nv_data, "", 8),
			NV_VARN(nv_ip4_typed, "dhcp_pool_start"),
			NV_VARN(nv_ip4_typed, "dhcp_pool_end"),
			NV_VARN(nv_ip4_typed, "dhcp_subnet_mask"),
			NV_VAR(nv_data, "", 4),
			NV_VARN(nv_ip4_typed, "router_ip"),
			NV_VARN(nv_ip4_typed, "dns_ip"),
			NV_VARN(nv_ip4_typed, "syslog_ip"),
			NV_VAR(nv_u32, "ttl"),
			NV_VAR(nv_data, "", 4),
			NV_VARN(nv_ip4_typed, "ip_2"),
			NV_VAR(nv_p8string, "domain"),
			NV_VAR(nv_data, "", 7),
			//NV_VARN(nv_ip4_typed, "ip_3"),
			NV_VARN(nv_array<nv_lan_addr_entry>, "lan_addrs", 16),
			//NV_VAR(nv_lan_addr_entry, "lan_addr_1"), // XXX make this an array
			NV_VAR(nv_data, "", 0x37a),
			NV_VAR(nv_array<nv_wan_dns_entry>, "wan_dns", 3),
		};
	}
};

class nv_group_csp : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_csp, "CSP.", "csp")

	protected:
	virtual list definition(int type, const nv_version& ver) const override
	{
		return {
			NV_VAR(nv_cdata<44>, ""),
			NV_VAR2(nv_enum<nv_u8>, "firewall_policy", "", { "low", "medium", "high" }),
		};
	}
};

class nv_group_fire : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_fire, "FIRE", "firewall")

	protected:
	virtual list definition(int type, const nv_version& ver) const override
	{
		return {
			NV_VAR(nv_data, "", 2),
			NV_VAR2(nv_bitmask<nv_u16>, "features", nv_bitmask<nv_u16>::valvec {
				"keyword_blocking",
				"domain_blocking",
				"http_proxy_blocking",
				"disable_cookies",
				"disable_java_applets",
				"disable_activex_ctrl",
				"disable_popups",
				"mac_tod_filtering",
				"email_alerts",
				"",
				"",
				"",
				""
				"block_fragmented_ip",
				"port_scan_detection",
				"syn_flood_detection"
			}),
			NV_VAR(nv_data, "", 4),
			NV_VAR(nv_u8, "word_filter_count"),
			NV_VAR(nv_data, "", 3),
			NV_VAR(nv_u8, "domain_filter_count"),
			NV_VAR2(nv_array<nv_fstring<0x20>>, "word_filters", 16, &is_empty_string),
			NV_VAR2(nv_array<nv_fstring<0x40>>, "domain_filters", 16, &is_empty_string),
			NV_VAR(nv_data, "", 0x2d4), // this theoretically gives room for 11 more domain filters
			NV_VAR(nv_data, "", 0xc),
			// 0x00 = all (!)
			// 0x01 = sunday
			// 0x40 = saturday
			NV_VAR(nv_bitmask<nv_u8>, "tod_filter_days"),
			NV_VAR(nv_data, "", 1),
			NV_VAR(nv_time_period, "tod_filter_time"),
			NV_VAR(nv_data, "", 0x2a80),
			NV_VAR(nv_ip4, "syslog_ip"),
			NV_VAR(nv_data, "", 2),
			// or nv_u8?
			// 0x08 = product config events
			// 0x04 = "known internet attacks"
			// 0x02 = blocked connections
			// 0x01 = permitted connections
			NV_VAR(nv_bitmask<nv_u16>, "syslog_events"),

		};
	}
};

class nv_group_cmev : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_cmev, "CMEV", "cmlog")

	protected:
	class nv_log_entry : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_log_entry, "log-entry")

		static bool is_end(const csp<nv_log_entry>& log_entry)
		{
			return log_entry->get("msg")->bytes() == 0;
		}

		virtual list definition() const override
		{
			return {
				NV_VAR(nv_data, "data", 8),
				NV_VAR(nv_timestamp, "time1"),
				NV_VAR(nv_timestamp, "time2"),
				NV_VAR(nv_p16string, "msg")
			};
		}
	};


	virtual list definition(int type, const nv_version& ver) const override
	{
		return {
			NV_VAR(nv_u8, "", true), // maybe a p16list??
			NV_VARN(nv_p8list<nv_log_entry>, "log"),
		};
	}
};

class nv_group_xxxl : public nv_group
{
	public:
	nv_group_xxxl(const string& magic)
	: nv_group(magic, bcm2dump::transform(magic, ::tolower))
	{}

	NV_GROUP_DEF_CLONE(nv_group_xxxl)

	protected:
	class nv_log_entry : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_log_entry, "log-entry")

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_timestamp, "time"),
				NV_VAR(nv_p16istring, "msg"),
				NV_VAR(nv_data, "", 2)
			};
		}
	};

	virtual list definition(int type, const nv_version& ver) const override
	{
		return {
			NV_VAR(nv_data, "", 1),
			NV_VARN(nv_p8list<nv_log_entry>, "log")
		};
	}
};

class nv_group_upc : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_upc, "UPC.", "upc")

	virtual list definition(int type, const nv_version& ver) const override
	{
		return {
			NV_VAR(nv_data, "", 3),
			NV_VAR(nv_bool, "parental_enable"),
			NV_VAR(nv_data, "", 6),
			NV_VAR(nv_u16, "parental_activity_time_enable"),
			NV_VAR(nv_zstring, "parental_password", 10),
			NV_VAR(nv_data, "", 0x2237),
			NV_VAR(nv_u8, "web_country"),
			NV_VAR(nv_u8, "web_language"),
			NV_VAR(nv_bool, "web_syslog_enable"),
			NV_VAR2(nv_bitmask<nv_u8>, "web_syslog_level", "", {
					"critical",
					"major",
					"minor",
					"warning",
					"inform"
			}),
			NV_VAR2(nv_array<nv_mac>, "trusted_macs", 10, &is_zero_mac),
			NV_VAR(nv_data, "", 0xd8),
			NV_VAR(nv_array<nv_ip4>, "lan_dns4_list", 3),
			NV_VAR(nv_array<nv_ip6>, "lan_dns6_list", 3),
		};
	}
};

class nv_group_xbpi : public nv_group
{
	public:
	nv_group_xbpi(bool ebpi)
	: nv_group(ebpi ? "Ebpi" : "bpi ", ebpi ? "ebpi" : "bpi")
	{}

	NV_GROUP_DEF_CLONE(nv_group_xbpi)

	virtual list definition(int format, const nv_version& ver) const override
	{
		if (format == fmt_dyn) {
			return {
				NV_VAR(nv_data, "code_access_start", 12),
				NV_VAR(nv_data, "cvc_access_start", 12),
				NV_VAR(nv_u8, "", true),
			};
			//return nv_group::definition(format, ver);
		}

		return {
#if 0
			NV_VAR3(format == fmt_dyn, nv_data, "code_access_start", 12),
			NV_VAR3(format == fmt_dyn, nv_data, "cvc_access_start", 12),
			NV_VAR3(format == fmt_dyn, nv_u8, "", true),
#endif
			NV_VAR(nv_p16data, "bpi_public_key"),
			NV_VAR(nv_p16data, "bpi_private_key"),
			NV_VAR(nv_p16data, "bpiplus_root_public_key"),
			NV_VAR(nv_p16data, "bpiplus_cm_certificate"),
			NV_VAR(nv_p16data, "bpiplus_ca_certificate"),
			NV_VAR(nv_p16data, "bpiplus_cvc_root_certificate"),
			NV_VAR(nv_p16data, "bpiplus_cvc_ca_certificate"),
		};
	}
};

class nv_group_rca : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_rca, "RCA ", "thomcm")

	virtual list definition(int format, const nv_version& ver) const override
	{
		if (format == fmt_perm) {
			return nv_group::definition(format, ver);
		}

		return {
			NV_VAR(nv_data, "", 2),
			NV_VAR(nv_p8list<nv_u32>, "scan_list_freqs"),
			NV_VAR(nv_u32, "us_alert_poll_period"),
			//NV_VAR(nv_data, "", 1),
			NV_VAR(nv_u32, "us_alert_thresh_dbmv_min"),
			NV_VAR(nv_u32, "us_alert_thresh_dbmv_max"),
			NV_VAR(nv_data, "", 1),
			NV_VAR(nv_p8data, "html_bitstring"),
			NV_VAR2(nv_enum<nv_u8>, "message_led", "", {
					"",
					"off",
					"on",
					"flashing"
			}),
			NV_VAR(nv_data, "", 5),
			NV_VAR(nv_p8list<nv_u32>, "last_known_freqs"),
			NV_VAR(nv_data, "", 2),

		};
	}
};

class nv_group_halif : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_halif, 0xf2a1f61f, "halif")

	virtual list definition(int format, const nv_version& ver) const override
	{
		if (format == fmt_dyn) {
			return {
				NV_VAR(nv_bool, "fap_bypass_enabled"),
				NV_VAR2(nv_bitmask<nv_u16>, "eth_auto_power_down_reg", "", nv_bitmask<nv_u16>::valvec {
						// actually a 4-bit integer, that is multiplied by 84 to give milliseconds
						"wakeup_timer_select_0",
						"wakeup_timer_select_1",
						"wakeup_timer_select_2",
						"wakeup_timer_select_3",
						// not set means a timeout of 2.7 seconds
						"auto_power_down_5400ms",
						"auto_power_down_enabled",
				}),
			};
		}

		return {
			NV_VAR2(nv_bitmask<nv_u32>, "interfaces", "", nv_bitmask<nv_u32>::valvec {
					"docsis",
					"ethernet",
					"hpna",
					"usb",
					"IP1",
					"IP2",
					"IP3",
					"IP4",
					"davic",
					"pci",
					"bluetooh",
					"802.11",
					"packet_generator",
					"IP5",
					"IP6",
					"IP7",
					"IP8",
					"wan_ethernet",
					"scb",
					"itc",
					"moca"

			}),
			NV_VAR(nv_mac, "cm_mac"),
			NV_VAR(nv_mac, "ip2_mac"),
			NV_VAR(nv_mac, "rg_mac"),
			NV_VAR(nv_mac, "ip4_mac"),
			NV_VAR(nv_u8, ""),
			NV_VAR(nv_u32, "default_hal_debug_zones"),
			NV_VAR(nv_u8, "board_rev"),
			NV_VAR(nv_u16, "usb_vid", true),
			NV_VAR(nv_u16, "usb_pid", true),
			NV_VAR(nv_mac, "usb_mac"),
			//NV_VAR(nv_bool, "usb_rndis"),
			NV_VAR2(nv_bitmask<nv_u8>, "features1", "", nv_bitmask<nv_u8>::valvec {
					"auto_negotiate",
					"full_duplex",
					"reject_cam_disabled",
			}),
			NV_VAR(nv_u16, "link_speed_mbps"),
			NV_VAR(nv_u32, "hpna_msg_level"),
			NV_VAR(nv_u8, "ds_tuner_type"),
			NV_VAR(nv_u8, "us_amp_type"),
			NV_VAR(nv_u32, "ds_reference_freq"),
			NV_VAR(nv_u32, "us_reference_freq"),
			NV_VAR(nv_u32, "phy_input_freq"),
			NV_VAR(nv_annex_mode, "annex_mode"),
			NV_VAR2(nv_bitmask<nv_u8>, "features2", "", nv_bitmask<nv_u8>::valvec {
					"watchdog",
					"bluetooth_master",
					"remote_flash_access",
					"fpm_token_depletion_watchdog_disabled",
			}),
			NV_VAR(nv_u8, "watchdog_timeout"),
			NV_VAR(nv_mac, "bluetooth_local_mac"),
			NV_VAR(nv_mac, "bluetooth_remote_mac"),
			//NV_VAR(nv_bool, "bluetooth_master"),
			NV_VAR(nv_data, "", 0x16),
			NV_VAR(nv_mac, "ip5_mac"),
			NV_VAR(nv_mac, "mta_mac"),
			NV_VAR(nv_mac, "veth_mac"),
			NV_VAR(nv_mac, "ip8_mac"),
			//NV_VAR(nv_u32, ""),
			NV_VAR(nv_u8, "spreader_scale"),
			NV_VAR(nv_u32, "us_sample_freq"),
			NV_VAR2(nv_bitmask<nv_u8>, "features3", "", nv_bitmask<nv_u8>::valvec {
					"1024qam",
					"docsis20_clipping",
					"proprietary_scdma_code_matrix",
					"advance_map_run_ahead_disabled"
					"",
					"",
					"",
					"us_priority_queue_disabled",
			}),
			NV_VAR(nv_u8, "ds_agi", true),
			NV_VAR(nv_u8, "ds_agt", true),
			NV_VAR(nv_u16, "stathr", true),
			NV_VAR(nv_u32, "stagi", true),
			NV_VAR(nv_u32, "stpga1", true),
			NV_VAR(nv_u32, "stagt", true),
			NV_VAR(nv_u16, "stabw1", true),
			NV_VAR(nv_u16, "stabw2", true),
			// actually a struct: { u16 bufsize, u16 buf_count }
			NV_VAR(nv_p8list<nv_cdata<4>>, "bcmalloc_settings"),
			NV_VAR(nv_u8, "num_shack_tries"),
			NV_VAR(nv_bool, "usb_rndis_driver"),
			NV_VAR(nv_bool, "power_save"),
			NV_VAR(nv_bool, "optimized_3420_freq_map"),
			NV_VAR(nv_bool, "high_output_pa"),
			NV_VAR(nv_u8, ""),
			NV_VAR2(nv_enum<nv_u8>, "diplexer_type", "", { "lowsplit", "midsplit", "" }),
			NV_VAR(nv_u8, "enabled_tuner_count"),
			NV_VAR(nv_bool, "cm_tuner_wideband"),
		};
	}
};

class nv_group_msc : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_msc, "MSC.", "msc")

	virtual list definition(int format, const nv_version& ver) const override
	{
		if (format == fmt_perm) {
			return nv_group::definition(format, ver);
		}

		return {
			NV_VAR(nv_data, "", 1),
			NV_VAR(nv_p8istring, "server_name"),
			NV_VAR(nv_data, "", 6),
			NV_VAR(nv_data, "", 2),
		};
	}
};

class nv_group_wigu : public nv_group
{
	public:
	nv_group_wigu(bool u)
	: nv_group(u ? "WiGu" : "WiGv", "guestwifi"s + (u ? "" : "2"))
	{}

	NV_GROUP_DEF_CLONE(nv_group_wigu);

	protected:
	// stuff NOT in first group:
	// ap_isolate
	// upnp_igdb
	// closed_network
	//
	// bss_maxassoc (not even in second group!)


	class nv_guest_wifi : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_guest_wifi, "guest-wifi");

		virtual list definition() const override
		{
			return {
				NV_VAR(nv_u8, ""),
				NV_VAR(nv_u8, ""),
				NV_VAR(nv_bool, "enabled"),
				NV_VAR(nv_fzstring<33>, "ssid"),
				NV_VAR(nv_u8, ""),
				NV_VAR(nv_u8, ""),
				NV_VAR(nv_group_8021::nv_wifi_encmode, "encryption"),
				NV_VAR(NV_ARRAY(nv_cdata<5>, 4), "wep64_keys"),
				NV_VAR(nv_u8, "wep_key_num"),
				NV_VAR(NV_ARRAY(nv_cdata<13>, 4), "wep128_keys"),
				NV_VAR(nv_fzstring<27>, "wep_key_passphrase"),
				NV_VAR(nv_data, "", 6),
				NV_VAR(nv_p8string, "wpa_psk"),
				NV_VAR(nv_u32_m<0xfffff>, "wpa_rekey_interval"),
				NV_VAR(nv_ip4, "radius_ip"),
				NV_VAR(nv_u16, "radius_port"),
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_p8string, "radius_key"),
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_bool, "shared_key_auth_required"),
				NV_VAR(nv_u8, "", true),
				NV_VAR(nv_group_8021::nv_wpa_auth, "wpa_auth"),
				NV_VAR(nv_data, "", 2),
				NV_VAR(nv_u16, "wpa_reauth_interval"),
				NV_VAR(nv_bool, "wpa2_preauth_enabled"),
				NV_VAR(nv_bool, "dhcp_enabled"),
				NV_VAR(nv_ip4, "dhcp_ip_address"),
				NV_VAR(nv_ip4, "dhcp_subnet_mask"),
				NV_VAR(nv_ip4, "dhcp_pool_start"),
				NV_VAR(nv_ip4, "dhcp_pool_end"),
				NV_VAR(nv_u32, "dhcp_lease_time"),
				NV_VAR(nv_u8, "network_bridge"),
				NV_VAR2(nv_enum<nv_u8>, "mac_policy", "mac_policy", { "disabled", "allow", "deny" }),
				NV_VAR(NV_ARRAY(nv_mac, 32), "mac_table"),
			};
		}
	};

	virtual list definition(int format, const nv_version& ver) const override
	{
		if (format == fmt_perm) {
			return nv_group::definition(format, ver);
		}

		return {
			NV_VAR(nv_guest_wifi, "net1"),
			NV_VAR(nv_guest_wifi, "net2"),
		};
	}
};
// TCH = Technicolor
class nv_group_tch : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_tch, "TCH ", "tch")

	virtual list definition(int format, const nv_version& ver) const override
	{
		if (format == fmt_perm) {
			return nv_group::definition(format, ver);
		}

		return {
			NV_VAR2(nv_enum<nv_u32>, "serial_console_mode", "", {
					"rw", "ro", "disabled"
			}),
		};
	}
};

// Scie / SA = Scientific Atlanta
class nv_group_scie : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_scie, "Scie", "sa")

	virtual list definition(int format, const nv_version& ver) const override
	{
		if (format == fmt_perm) {
			return {
				NV_VAR(nv_u8, "ds_search_plan"),
				NV_VAR(nv_u32, "" /*"comcast_led_mode"*/),
				NV_VAR(nv_u8, ""),
				NV_VAR(nv_u8, ""),
				NV_VAR(nv_fzstring<0x100>, "hw_model"),
				NV_VAR(nv_fzstring<0x100>, "hw_version"),
				NV_VAR(nv_fzstring<0x100>, "hw_serial"),
				NV_VAR(nv_fzstring<0x100>, "hw_cm_mac"),
			};
		}

		return {
			NV_VAR(nv_u32, "access_protect_delay"),
			NV_VAR2(nv_enum<nv_u8>, "access_protect_mode", "", {
				"disabled", "enabled", "auto",
			}),
			NV_VAR2(nv_bitmask<nv_u32>, "features", {
					/* 0x01 */ "igmp_proxy",
					/* 0x02 */ "web_switch",
					/* 0x04 */ "ftp_improvement_switch",
					/* 0x08 */ "console_enabled",
					/* 0x10 */ "",
					/* 0x20 */ "console_read_only",
			}),
			NV_VAR(nv_cdata<0x28>, ""),
			NV_VAR(NV_ARRAY(nv_u32, 10), ""),
			NV_VAR(NV_ARRAY(nv_u8, 2), ""),
			//
			NV_VAR(nv_timestamp, "mta_certificate_date"),
			//
			NV_VAR2(nv_enum<nv_u8>, "dhcp_requirements", "", {
					"use_opt_122",
					"require_opt_122_or_177",
					"use_opt_177"
			}),
			NV_VAR(nv_p8string, "telnet_username"),
			NV_VAR(nv_p8string, "telnet_password"),
			NV_VAR(nv_u8, ""),
			NV_VAR(nv_u8, ""),
			NV_VAR(NV_ARRAY(nv_u8, 2), "mta_off_hook_current"),
			NV_VAR2(nv_enum<nv_u8>, "telnet_mode", "", {
					"disabled", "enabled", "enabled_by_mib",
			}),
			NV_VAR(nv_u8, ""),
			NV_VAR(NV_ARRAY(nv_u8, 2), "mta_off_hook_power"),
			NV_VAR(nv_bool, "mta_fast_busy_signal"),
			NV_VAR(nv_u8, ""),
			NV_VAR(nv_u32, "batt_measured_capacity"),
			NV_VAR(NV_ARRAY(nv_u32, 5), ""),
			NV_VAR(nv_u32, "batt_avg_discharge_current"),
#if 0
#if 1
			NV_VAR(NV_ARRAY(nv_sa_audit_entry, 60), "audit_log"),
			NV_VAR(nv_cdata<0x10d8>, ""),
			NV_VAR(nv_u32, ""),
			NV_VAR(nv_u8, ""),
#endif
			NV_VAR(NV_ARRAY(nv_sa_event_entry, 100), "event_log"),
			NV_VAR(nv_u16, "event_log_head"),
			NV_VAR(nv_u16, "event_log_tail"),
			NV_VAR(nv_cdata<0x99>, ""),
			NV_VAR(nv_u8, ""),
			NV_VAR(nv_u32, ""),
			NV_VAR(nv_u32, ""),
			NV_VAR(nv_u32, ""),
			NV_VAR(nv_u32, ""),
			NV_VAR(nv_u32, ""),
			NV_VAR(nv_cdata<9>, ""),
			NV_VAR(nv_cdata<9>, ""),
			NV_VAR(nv_cdata<9>, ""),
			NV_VAR(nv_cdata<9>, ""),
			NV_VAR(nv_cdata<7>, ""),
			NV_VAR(nv_cdata<7>, ""),
			NV_VAR(nv_u32, ""),
			NV_VAR(nv_u8, ""),
			NV_VAR(nv_u8, ""),
			NV_VAR(nv_u8, ""),
			NV_VAR(nv_u8, ""),
#else
			//NV_VAR(nv_cdata<0x435d>, ""),
			NV_VAR(nv_cdata<0x435c>, ""),
#endif
			//
			NV_VAR(nv_bool, "mta_rtp_mute_on_local_ring_back"),
			NV_VAR(nv_u32, "cm_timer4"),
			NV_VAR(nv_bool, "mta_force_t38"),
#if 1
			NV_VAR2(nv_enum<nv_u32>, "serial_console_mode", "", {
					"rw", "ro", "disabled"
			}),
#else
			NV_VAR(nv_u32, "console_status", true),
#endif
			NV_VAR(nv_bool, "multicast_promiscous_mode"),
			NV_VAR(nv_u32, "docsis1x_qpsk_burst_preamble"),
			//
			NV_VAR(nv_fzstring<0x41>, "last_upgrade_sw_name"),
		};
	}

	protected:
	class nv_sa_audit_entry : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_sa_audit_entry, "sa-audit-entry")

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_u32, ""),
				NV_VAR(nv_u32, ""),
				NV_VAR(nv_u32, ""),
				NV_VAR(nv_cdata<0xf>, ""),
				NV_VAR(nv_cdata<0x1e>, ""),
				NV_VAR(nv_cdata<0x1e>, ""),
				NV_VAR(nv_cdata<0x7f>, ""),
			};
		}
	};

	class nv_sa_event_entry : public nv_compound
	{
		public:
		NV_COMPOUND_DEF_CTOR_AND_TYPE(nv_sa_event_entry, "sa-event-entry")

		protected:
		virtual list definition() const override
		{
			return {
				NV_VAR(nv_cdata<0xb>, ""),
				NV_VAR(nv_u32, ""),
				NV_VAR(nv_cdata<0x7f>, ""),
				NV_VAR(nv_cdata<6>, ""),
				NV_VAR(nv_u32, ""),
				NV_VAR(nv_u16, ""),
				NV_VAR(nv_u32, ""),
				NV_VAR(nv_u16, ""),
				NV_VAR(nv_u32, ""),
				NV_VAR(nv_u16, ""),
				NV_VAR(nv_u32, ""),
			};
		}
	};
};

class nv_group_fact : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_fact, "FACT", "fact")

	virtual list definition(int format, const nv_version& ver) const override
	{
		if (format == fmt_dyn) {
			return nv_group::definition(format, ver);
		}

		return {
			NV_VAR(nv_u16, "private_mib_enabled"),
			NV_VAR(NV_ARRAY(nv_p16string, 8), "enable_keys"),
			NV_VAR(NV_ARRAY(nv_p16string, 4), "serial_numbers"),
			NV_VAR(nv_bool, "temporary_mib_enabled"),
		};
	}
};

class nv_group_docsis : public nv_group
{
	public:
	NV_GROUP_DEF_CTOR_AND_CLONE(nv_group_docsis, 0xd0c20100, "docsis1")

	protected:
	virtual list definition(int type, const nv_version& ver) const override
	{
		if (type == fmt_dyn) {
			return {
				NV_VAR(nv_u32, "ds_frequency"),
				NV_VAR(nv_u8, "us_channel"),
				NV_VAR(nv_u16, "us_power"),
				NV_VAR(nv_u32, "startup_ds_frequency"),
				NV_VAR(nv_u8, "startup_us_channel"),
				NV_VAR(nv_u8, "startup_context"),
				NV_VAR(nv_bool, "docsis10_igmp_multicast_enabled"),
				NV_VAR(nv_u16, ""),
				NV_VAR(nv_u32, ""),
				NV_VAR(nv_u8, ""),
				NV_VAR(nv_u8, ""),
				NV_VAR(nv_u8, ""),
				NV_VAR(nv_u8, ""),
				NV_VAR(nv_annex_mode, "annex_mode_of_last_good_ds"),
			};
		} else {
			return {
				NV_VAR2(nv_bitmask<nv_u32>, "features", nv_bitmask<nv_u32>::valvec {
						"single_ds_channel",   // 0x00000001
						"bpi",                 // 0x00000002
						"concat",              // 0x00000004
						"fragmentation",       // 0x00000008
						"phs",                 // 0x00000010
						"igmp",                // 0x00000020
						"rate_shaping",        // 0x00000040
						"",                    // 0x00000080
						"dhcp",                // 0x00000100
						"time_of_day",         // 0x00000200
						"config_file_tftp",    // 0x00000400
						"canned_registration", // 0x00000800
						"online_rng_rsp",      // 0x00001000
						"",                    // 0x00002000
						"bpi23_secure_sw_dload_disabled", // 0x00004000
						"docsis_20_hack_for_1x_cmts", // 0x00008000
						"",                    // 0x00010000
						"force_config_file",   // 0x00020000
						"ack_cel_technology",  // 0x00040000
						"non_standard_upstream", // 0x00080000
						"",                    // 0x00100000
						"ds_header_detection", // 0x00200000
						"scan_up_annex_a",     // 0x00400000
				}),
				NV_VAR(nv_u8, "upstream_queues_enabled"),
				NV_VAR(nv_u8, "bpi_version"),
				NV_VAR(nv_u32, "disabled_docsis_timers"),
				NV_VAR(nv_u8, "concat_threshold"),
				NV_VAR(nv_u8, "rate_shaping_time_interval"),
				NV_VAR(nv_u32, ""),
				NV_VAR(nv_ip4, "canned_ip_addr"),
				NV_VAR(nv_ip4, "canned_ip_mask"),
				NV_VAR(nv_ip4, "canned_gateway_ip"),
				NV_VAR(nv_ip4, "canned_tftp_ip"),
				NV_VAR(nv_fzstring<0x40>, "canned_cm_config_file"),
				NV_VAR(nv_ip4, "time_server_ip"),
				NV_VAR(nv_ip4, "log_server_ip"),
				NV_VAR(nv_ipstack, "ipstack_number"),
				NV_VAR(nv_u8, "max_ugs_queue_depth"),
				NV_VAR(nv_u8, "max_concat_packets"),
				NV_VAR(nv_p8list<nv_cdata<3>>, "ugs_queue"),
				NV_VAR(nv_u16, "default_ranging_class"),
			};
		}
	}
};

struct registrar {
	registrar()
	{
		vector<sp<nv_group>> groups = {
			NV_GROUP(nv_group_cmap),
			NV_GROUP(nv_group_mlog),
			NV_GROUP(nv_group_8021, false),
			NV_GROUP(nv_group_8021, true),
			NV_GROUP(nv_group_t802),
			NV_GROUP(nv_group_rg),
			NV_GROUP(nv_group_cdp),
			NV_GROUP(nv_group_csp),
			NV_GROUP(nv_group_fire),
			NV_GROUP(nv_group_cmev),
			NV_GROUP(nv_group_upc),
			NV_GROUP(nv_group_xxxl, "RSTL"),
			NV_GROUP(nv_group_xxxl, "CMBL"),
			NV_GROUP(nv_group_xxxl, "EMBL"),
			NV_GROUP(nv_group_thom),
			NV_GROUP(nv_group_xbpi, false),
			NV_GROUP(nv_group_xbpi, true),
			NV_GROUP(nv_group_halif),
			NV_GROUP(nv_group_rca),
			NV_GROUP(nv_group_msc),
			NV_GROUP(nv_group_wigu, false),
			NV_GROUP(nv_group_wigu, true),
			NV_GROUP(nv_group_scie),
			NV_GROUP(nv_group_fact),
			NV_GROUP(nv_group_snmp),
			NV_GROUP(nv_group_docsis),
			NV_GROUP(nv_group_tch),
		};

		for (auto g : groups) {
			nv_group::registry_add(g);
		}
	}
} instance;
}
}
