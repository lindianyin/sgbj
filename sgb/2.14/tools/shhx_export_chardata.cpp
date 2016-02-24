
/*

����һ����ɫ�����ݵ�һ���ļ���

*/

void export(const std::string& prefix, int cid)
{
    /*�ʺ���Ϣ*/
    Query q(GetDb());
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    CharData* pc = cdata.get();
    if (pc)
    {
        json_spirit::Object cobj;
        
        q.get_result("select qid,qname,union_id,server_id,money from accounts where account='" + pc->m_account + "'");
        if (!q.fetch_row())
        {
            q.free_result();
            return HC_ERROR_WRONG_ACCOUNT;
        }
        //�ʺ���Ϣ
        json_spirit::Object account;
        account.push_back( Pair("account", pc->m_account));
        account.push_back( Pair("qid", q.getstr()));
        account.push_back( Pair("qname", q.getstr()));
        account.push_back( Pair("qid", q.getval()));
        account.push_back( Pair("server_id", q.getstr()));
        account.push_back( Pair("money", q.getval()));
        cobj.push_back( Pair("account", account));

        //��ɫ��Ϣ
        json_spirit::Object charactor;
        //charactor.push_back( Pair("account", pc->m_account) );
        charactor.push_back( Pair("name", pc->m_name) );
        charactor.push_back( Pair("spic", pc->m_spic) );
        charactor.push_back( Pair("level", pc->m_level) );
        charactor.push_back( Pair("lastlogin", pc->m_login_time) );
        charactor.push_back( Pair("state", pc->m_state) );
        charactor.push_back( Pair("delete_time", pc->m_deleteTime) );
        charactor.push_back( Pair("createTime", pc->m_createTime) );
        charactor.push_back( Pair("chenmi_time", pc->m_chenmi_time) );
        charactor.push_back( Pair("continue_days", pc->m_continue_days) );
        cobj.push_back( Pair("charactors", charactor) );

        //��ʯ��Ϣ char_baoshi
        if (pc->m_baoshi.size() > 0)
        {
            json_spirit::Array char_baoshi;
            for (std::map<int, boost::shared_ptr<baowuBaoshi> >::iterator it = pc->m_baoshi.begin(); it != pc->m_baoshi.end(); ++it)
            {
                baowuBaoshi* pbaoshi = it->second.get();
                //ֻ����û���佫���ϵı�ʯ
                if (pbaoshi && pbaoshi->m_general.get() == NULL)
                {
                    json_spirit::Object baoshi;
                    //baoshi.push_back( Pair("cid", cid) );
                    baoshi.push_back( Pair("type", pbaoshi->m_baseBaoshi->type) );
                    for (int i = 0; i < pbaoshi->m_attr_num; ++i)
                    {
                        baoshi.push_back( Pair("attr_type" + LEX_CAST_STR(i+1), pbaoshi->m_attributes[i].type) );
                        baoshi.push_back( Pair("attr_value" + LEX_CAST_STR(i+1), pbaoshi->m_attributes[i].value) );
                        baoshi.push_back( Pair("attr_color" + LEX_CAST_STR(i+1), pbaoshi->m_attributes[i].color) );
                    }
                    char_baoshi.push_back(baoshi);
                }
            }
            cobj.push_back( Pair("char_baoshi", char_baoshi) );
        }
        //������½�����ȡ��Ϣ
        if (pc->m_login_present.size()>0)
        {
            json_spirit::Array loginPresent;
            for (std::map<int,CharLoginPresent>::iterator it = pc->m_login_present.begin(); it != pc->m_login_present.end(); ++it)
            {
                json_spirit::Object obj;
                obj.push_back( Pair("pid", it->second.present->id) );
                obj.push_back( Pair("state", it->second.state) );
                loginPresent.push_back(obj);
            }
            cobj.push_back( Pair("char_continue_login_present", loginPresent) );
        }

        //��ɫ��Ҫ���� char_data
        {
            json_spirit::Object obj;
            obj.push_back( Pair("camp", pc->m_camp) );
            obj.push_back( Pair("levelupTime", pc->m_levelupTime) );
            obj.push_back( Pair("mapid", pc->m_area) );
            obj.push_back( Pair("backPack", pc->m_backpack.m_max_size) );
            obj.push_back( Pair("freeRest", pc->m_free_rest) );
            obj.push_back( Pair("freeRestTime", (int)pc->m_free_rest_time) );
            obj.push_back( Pair("finish_first_explore", pc->m_first_explore) );
            obj.push_back( Pair("finish_second_explore", pc->m_second_explore) );
            obj.push_back( Pair("vip", pc->m_vip) );
            obj.push_back( Pair("prestige", pc->m_prestige) );
            obj.push_back( Pair("official", pc->m_offical) );
            obj.push_back( Pair("cLevel", pc->m_level) );
            obj.push_back( Pair("chat", pc->m_chat) );

            cobj.push_back( Pair("char_data", obj) );
        }
        //��ɫ��������char_data_extra
        {
            json_spirit::Array extras;
            for (std::map<int, int>::itrator it = pc->m_normal_extra_data.begin(); it != pc->m_normal_extra_data.end(); ++it)
            {
                json_spirit::Object obj;
                obj.push_back( Pair("type", 0) );
                obj.push_back( Pair("field", it->first) );
                obj.push_back( Pair("value", it->second) );
                extras.push_back(obj);
            }
            for (std::map<int, int>::itrator it = pc->m_daily_extra_data.begin(); it != pc->m_daily_extra_data.end(); ++it)
            {
                json_spirit::Object obj;
                obj.push_back( Pair("type", 1) );
                obj.push_back( Pair("field", it->first) );
                obj.push_back( Pair("value", it->second) );
                extras.push_back(obj);
            }
            for (std::map<int, int>::itrator it = pc->m_week_extra_data.begin(); it != pc->m_week_extra_data.end(); ++it)
            {
                json_spirit::Object obj;
                obj.push_back( Pair("type", 2) );
                obj.push_back( Pair("field", it->first) );
                obj.push_back( Pair("value", it->second) );
                extras.push_back(obj);
            }
            cobj.push_back( Pair("char_data_extra", extras) );
        }
        //������ȡ���
        if (pc->m_welfare)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("welfare", pc->m_welfare) );
            cobj.push_back( Pair("char_data_temp", obj) );
        }
        //ȱʡ����
        {
            cobj.push_back( Pair("char_default_zhen", pc->m_zhens.m_default_zhen) );
        }
        //װ����Ϣchar_equipts
        {
            json_spirit::Array equipts;
            for (std::map<int, boost::shared_ptr<EquipmentData> >::iterator it = pc->m_backpack.equipments.equipments.begin(); it != pc->m_backpack.equipments.equipments.end(); ++it)
            {
                EquipmentData* pe = it->second.get();
                if (pe)
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("base_id", pe->baseid) );
                    obj.push_back( Pair("qLevel", pe->qLevel) );
                    obj.push_back( Pair("addAttr", pe->value) );
                    obj.push_back( Pair("inheritTimes", pe->inheritTimes) );
                    obj.push_back( Pair("beinheritTimes", pe->beinheritTimes) );
                    obj.push_back( Pair("state", pe->state) );
                    obj.push_back( Pair("deleteTime", pe->deleteTime) );
                    equipts.push_back(obj);
                }
            }
            cobj.push_back( Pair("char_equipts", equipts) );
        }
        //̽����Ϣ

        //������Ϣ

        //�佫��Ϣ
        /*char_generals`
            (`id`, `gid`, `cid`, `level`, `tong`, `str`, `wisdom`, `color`, `state`, `delete_time`,
            `fac_a`, `fac_b`, `fac_a_max`, `fac_b_max`, `add_level`, `add_str`, `add_int`, `add_tong`,
            `baowu_level`, `reborn_times`, `nickname`, `genius1`, `genius2`, `genius3`, `genius4`, `genius5`)*/
        {
            json_spirit::Array generals;
            for (std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = pc->m_generals.m_generals.begin(); it != pc->m_generals.m_generals.end(); ++it)
            {
                if (it->second.get())
                {
                    CharGeneralData* pg = it->second.get();
                    json_spirit::Object g;
                    g.push_back( Pair("gid", pg->m_gid) );
                    g.push_back( Pair("level", pg->m_level) );
                    g.push_back( Pair("tong", pg->m_tongyu) );
                    g.push_back( Pair("str", pg->m_str) );
                    g.push_back( Pair("wisdom", pg->m_int) );
                    g.push_back( Pair("color", pg->m_color) );
                    g.push_back( Pair("state", pg->m_state) );
                    g.push_back( Pair("delete_time", pg->m_delete_time) );
                    g.push_back( Pair("fac_a", pg->m_chengzhang[0]) );
                    g.push_back( Pair("fac_b", pg->m_chengzhang[1]) );
                    g.push_back( Pair("fac_a_max", pg->m_chengzhang_max[0]) );
                    g.push_back( Pair("fac_b_max", pg->m_chengzhang_max[1]) );
                    g.push_back( Pair("add_level", pg->m_add) );
                    g.push_back( Pair("add_str", pg->m_wash_str) );
                    g.push_back( Pair("add_int", pg->m_wash_int) );
                    g.push_back( Pair("add_tong", pg->m_wash_tong) );

                    g.push_back( Pair("bauwu_level", pg->m_baowu_level) );
                    g.push_back( Pair("reborn_times", pg->m_reborn_times) );
                    g.push_back( Pair("wash_times", pg->m_wash_times) );
                    g.push_back( Pair("nickname", pg->b_nickname) );

                    //�츳
                    for (int i = 0; i < pg->m_genius.size(); ++i)
                    {
                        g.push_back( Pair("genius" + LEX_CAST_STR(i+1), pg->m_genius[i]) );
                    }
                    json_spirit::Array gbaoshi;
                    //��ʯ
                    for (std::vector<boost::shared_ptr<baowuBaoshi> >::iterator it = pg->m_baoshi.begin(); it != pg->m_baoshi.end(); ++it)
                    {
                        baowuBaoshi* pbaoshi = it->get();
                        //ֻ����û���佫���ϵı�ʯ
                        if (pbaoshi)
                        {
                            json_spirit::Object baoshi;
                            //baoshi.push_back( Pair("cid", cid) );
                            baoshi.push_back( Pair("type", pbaoshi->m_baseBaoshi->type) );
                            for (int i = 0; i < pbaoshi->m_attr_num; ++i)
                            {
                                baoshi.push_back( Pair("attr_type" + LEX_CAST_STR(i+1), pbaoshi->m_attributes[i].type) );
                                baoshi.push_back( Pair("attr_value" + LEX_CAST_STR(i+1), pbaoshi->m_attributes[i].value) );
                                baoshi.push_back( Pair("attr_color" + LEX_CAST_STR(i+1), pbaoshi->m_attributes[i].color) );
                            }
                            gbaoshi.push_back(baoshi);
                        }
                    }
                    if (gbaoshi.size() > 0)
                    {
                        g.push_back( Pair("baoshi", gbaoshi) );
                    }
                    generals.push_back(g);
                }
            }
            cobj.push_back( Pair("char_generals", generals) );
        }

        //ս����Ϣ
        /*
        `char_horses` (`cid`, `horseid`, `exp`, `pugong`, `pufang`, `cegong`, `cefang`, `bingli`, `action_start`, `action_end`)
        */
        {
            json_spirit::Object horse;
            horse.push_back( Pair("horseid", pc->m_horse.horseid) );
            horse.push_back( Pair("exp", pc->m_horse.exp) );
            horse.push_back( Pair("pugong", pc->m_horse.pugong) );
            horse.push_back( Pair("pufang", pc->m_horse.pufang) );
            horse.push_back( Pair("cegong", pc->m_horse.cegong) );
            horse.push_back( Pair("cefang", pc->m_horse.cefang) );
            horse.push_back( Pair("bingli", pc->m_horse.bingli) );

            horse.push_back( Pair("action_start", pc->m_horse.start_time) );
            horse.push_back( Pair("action_end", pc->m_horse.end_time) );
            cobj.push_back( Pair("char_horses", horse) );
        }

        /*ս��
        `char_horses_action` (`cid`, `horse_id`, `fruit_id`, `state`, `start_time`, `end_time`)
        */
        if (pc->m_horse.action_list.size())
        {
            json_spirit::Array horse_action;
            for (std::vector<CharHorseFruitAction>::iterator it = pc->m_horse.action_list.begin(); it != pc->m_horse.action_list.end(); ++it)
            {
                if (it->fruits_list.size())
                {
                    for (std::vector<CharHorseFruit>::iterator it2 = it->fruits_list.begin(); it2 != it->fruits_list.end(); ++it2)
                    {
                        json_spirit::Object f;
                        f.push_back( Pair("horse_id", it->horse_id) );
                        f.push_back( Pair("fruit_id", it2->fruit->id) );
                        f.push_back( Pair("state", it2->state) );
                        f.push_back( Pair("start_time", it2->start_time) );
                        f.push_back( Pair("end_time", it2->end_time) );

                        horse_action.push_back(f);
                    }
                }
            }
            if (horse_action.size())
            {
                cobj.push_back( Pair("char_horses_action", horse_action) );
            }
        }

        /*
        ��ͼ������ȡ��Ϣ
        `char_map_intro_get` (`cid`, `mapid`, `get`)
        */
        if (pc->m_map_intro_get.size())
        {
            json_spirit::Array gets;
            for (std::map<int,int>::iterator it = pc->m_map_intro_get.begin(); it != pc->m_map_intro_get.end(); ++it)
            {
                json_spirit::Object g;
                g.push_back( Pair("mapid", it->first) );
                g.push_back( Pair("get", it->second) );
                gets.push_back(g);
            }
            cobj.push_back( Pair("char_map_intro_get", gets) );
        }

        /*
        ���ֳ��Ž�����ȡ��Ϣ
        `char_newbie_event` (`cid`, `level`)
        */
        if (pc->m_newbie_reward.size())
        {
            json_spirit::Array newbie_r;
            for (std::map<int,bool>::iterator it = pc->m_newbie_reward.begin(); it != pc->m_newbie_reward.end(); ++it)
            {
                if (it->second)
                {
                    json_spirit::Object g;
                    g.push_back( Pair("level", it->first) );
                    newbie_r.push_back(g);
                }
            }
            cobj.push_back( Pair("char_newbie_event", newbie_r) );
        }

        /*
        ����������Ϣ
        `char_new_states`
        (`cid`, `star_map`, `star1`, `star2`, `star3`, `star4`, `star5`, `star6`, `star7`, `star8`, `state1`, `state2`, `state3`)
        */
        {
            json_spirit::Object state;
            state.push_back( Pair("star_map", pc->m_newStates._star_level) );
            for (int i = 0; i < 8; ++i)
            {
                state.push_back( Pair("star" + LEX_CAST_STR(i+1), pc->m_newStates._stars[i]) );
            }
            for (int i = 0; i < 3; ++i)
            {
                state.push_back( Pair("state" + LEX_CAST_STR(i+1), pc->m_newStates._s[i]) );
            }
            cobj.push_back( Pair("char_new_states", state) );
        }

        /*
        ������Ϣ
        `char_new_weapons` (`cid`, `wType`, `wid`, `level`)
        */
        {
            json_spirit::Array weapons;
            for (int i = 0; i < 5; ++i)
            {
                json_spirit::Object o;
                o.push_back( Pair("wType", pc->m_new_weapons._weapons[i]._type) );
                o.push_back( Pair("wid", pc->m_new_weapons._weapons[i]._baseWeapon->_id) );
                o.push_back( Pair("wid", pc->m_new_weapons._weapons[i]._level) );
                weapons.push_back(o);
            }
            if (weapons.size())
            {
                cobj.push_back( Pair("char_new_weapons", weapons) );
            }
        }

        /*
        ��ְ����
        `char_offical_skills` (`cid`, `sid`, `level`)
        */
        if (pc->m_officalskill_list.size())
        {
            json_spirit::Array ss;
            for (std::map<int, boost::shared_ptr<officalskills> >::iterator it = pc->m_officalskill_list.begin(); it != pc->m_officalskill_list.end(); ++it)
            {
                if (it->second.get())
                {
                    json_spirit::Object s;
                    s.push_back( Pair("sid", it->second->sid) );
                    s.push_back( Pair("level", it->second->level) );
                    ss.push_back(s);
                }
            }
            cobj.push_back( Pair("char_offical_skills", ss) );
        }

        /*
        �����ȡ��¼
        `char_opened_packs` (`cid`, `pid`, `code`, `seqNo`, `open_time`)
        */
        {
            json_spirit::Array a;
            packsMgr::getInstance()->export_opened(cid, a);
            if (a.size())
            {
                cobj.push_back( Pair("char_opened_packs", a) );
            }
        }

        /*�����һ���¼
        `char_recharge` (`id`, `cid`, `account`, `gold`, `input`, `type`)
        */
        {
            json_spirit::Array rs;
            Query q(GetDb());
            int total_recharge = 0;
            q.get_result("select gold,input,type from char_recharge where cid='" + LEX_CAST_STR(cid) + "'");
            while (q.fetch_row())
            {
                json_spirit::Object o;
                int gold = q.getval();
                total_recharge += gold;
                o.push_back( Pair("gold", gold) );
                o.push_back( Pair("input", q.getstr()) );
                o.push_back( Pair("type", q.getstr()) );
                rs.push_back(o);
            }
            q.free_result();
            if (total_recharge < pc->m_total_recharge)
            {
                std::string rtime;
                q.get_result("select now()");
                if (q.fetch_row())
                {
                    rtime = q.getstr();
                    q.free_result();
                }
                else
                {
                    rtime = "";
                    q.free_result();
                }
                json_spirit::Object o;
                o.push_back( Pair("gold", pc->m_total_recharge-total_recharge) );
                o.push_back( Pair("input", rtime) );
                o.push_back( Pair("type", "convert") );
                rs.push_back(o);
            }
            if (rs.size())
            {
                cobj.push_back( Pair("char_recharge", rs) );
            }
        }

        /*���ʳ�ֵ����
        `char_recharge_event` (`cid`, `id`, `num`)
        */
        if (pc->m_recharge_reward.size())
        {
            json_spirit::Array a;
            for (std::map<int,int>::iterator it = pc->m_recharge_reward.begin(); it != pc->m_recharge_reward.end(); ++it)
            {
                json_spirit::Object o;
                o.push_back( Pair("id", it->first) );
                o.push_back( Pair("num", it->second) );
                a.push_back(o);
            }
            cobj.push_back( Pair("char_recharge_event", a) );
        }

        /*��Դ��Ϣ
        `char_resource` (`cid`, `gold`, `silver`, `ling`, `rtime`, `explore_ling`)
        */
        {
            json_spirit::Object a;
            a.push_back( Pair("gold", pc->m_gold) );
            a.push_back( Pair("silver", pc->m_gold) );
            a.push_back( Pair("ling", pc->m_ling) );
            a.push_back( Pair("feat", pc->m_feat) );
            a.push_back( Pair("explore_ling", pc->m_explore_ling) );
            cobj.push_back( Pair("char_resource", a) );
        }

        /*�Ҷ������Ϣ*/

        /*�����̵���Ϣ*/

        /*������Ϣ
        `char_skills` (`cid`, `sid`, `level`, `exp`)
        */
        if (pc->m_skill_list.size())
        {
            json_spirit::Array skills;
            for (std::map<int, boost::shared_ptr<charSkill> >::iterator it = pc->m_skill_list.begin(); it != pc->m_skill_list.end(); ++it)
            {
                if (it->second.get())
                {
                    json_spirit::Object s;
                    s.push_back( Pair("sid", it->first) );
                    s.push_back( Pair("level", it->second->level) );
                    s.push_back( Pair("exp", it->second->exp) );
                    skills.push_back(s);
                }
            }
            cobj.push_back( Pair("char_skills", skills) );
        }

        /*ұ����Ϣ
        `char_smelt` (`cid`, `pos`, `state`, `start_time`, `end_time`, `left_min`, `quality`, `out_put`, `cost_ling`, `cost_hour`, `cost_ore`)
        */
        boost::shared_ptr<charSmeltData> sp_smelt = SmeltMgr::getInstance()->getCharSmeltData(cdata->m_id);
        if (!sp_smelt.get())
        {
            json_spirit::Array smelt_list;
            charSmeltData& smelt = *(sp_smelt.get());
            for(int i = 0; i < smelt_queue_max; ++i)
            {
                json_spirit::Object smeltObj;
                smeltObj.push_back( Pair("pos", smelt.SmeltList[i].pos) );
                smeltObj.push_back( Pair("state", smelt.SmeltList[i].state) );
                smeltObj.push_back( Pair("start_time", smelt.SmeltList[i].start_time) );
                smeltObj.push_back( Pair("end_time", smelt.SmeltList[i].end_time) );
                
                if (smelt.SmeltList[i].state == 2)
                {
                    smeltObj.push_back( Pair("quality", smelt.SmeltList[i].smelt.quality) );
                    smeltObj.push_back( Pair("out_put", smelt.SmeltList[i].smelt.out_put) );
                    smeltObj.push_back( Pair("cost_hour", smelt.SmeltList[i].smelt.cost_hour) );
                    if (smelt.SmeltList[i].smelt.cost_ling)
                    {
                        smeltObj.push_back( Pair("cost_ling", smelt.SmeltList[i].smelt.cost_ling) );
                    }
                    smeltObj.push_back( Pair("cost_ore", smelt.SmeltList[i].smelt.cost_ore) );

                }
                smelt_list.push_back(smeltObj);
            }
            cobj.push_back( Pair("char_smelt", smelt_list) );
        }
        
        /*����������Ϣ*/

        /*�ؿ���Ϣ
        `char_stronghold` (`cid`, `mapid`, `stageid`, `pos1`, `pos2`, `pos3`, `pos4`, `pos5`, `pos6`, `pos7`, `pos8`, `pos9`, `pos10`, `pos11`,
        `pos12`, `pos13`, `pos14`, `pos15`, `pos16`, `pos17`, `pos18`, `pos19`, `pos20`, `pos21`, `pos22`, `pos23`, `pos24`, `pos25`)
        */
        {
            json_spirit::Array a;
            for (std::map<int, boost::shared_ptr<CharMapData> >::iterator it = pc->m_tempo.CharMapsData.begin(); it != pc->m_tempo.CharMapsData.end(); ++it)
            {
                if (it->second.get())
                {
                    CharMapData& pm = *(it->second.get());
                    for (std::map<int, boost::shared_ptr<CharStageData> >::iterator it2 = pm.begin(); it2 != pm.end(); ++it2)
                    {
                        CharStageData* ps = it->second;
                        if (ps)
                        {
                            json_spirit::Object o;
                            for (int i = 0; i < 25; ++i)
                            {
                                o.push_back( Pair("mapid", it->first) );
                                o.push_back( Pair("stageid", it2->first) );
                                if (ps->m_stronghold[i].get())
                                {
                                    CharStrongholdData* cs = ps->m_stronghold[i].get();
                                    o.push_back( Pair("pos" + LEX_CAST_STR(i+1), cs->m_state) );
                                }
                                else
                                {
                                    o.push_back( Pair("pos" + LEX_CAST_STR(i+1), -2) );
                                }
                            }
                            a.push_back(o);
                        }
                    }
                }
                if (a.size())
                {
                    cobj.push_back( Pair("char_stronghold", a) );
                }
            }
        }
        
        /*�ؿ�״̬��Ϣ*/

        /*�ؿ��������*/

        /**/

        /*ɨ����� ɨ������*/

        /*������Ϣ
        `char_tasks` (`cid`, `tid`, `state`)
        */
        {
            json_spirit::Object t;
            t.push_back( Pair("tid", pc->m_task.tid) );
            t.push_back( Pair("state", pc->m_task.done ? 1 : 0) );
            cobj.push_back( Pair("char_tasks", t) );
        }

        /*�ۼƳ�ֵ
        `char_total_recharge` (`cid`, `total_recharge`)
        */
        {
            cobj.push_back( Pair("char_total_recharge", pc->m_total_recharge) );
        }
        
        /*�佫ѵ����*/
        
        /*�佫ѵ��λ
        `char_train_place` (`cid`, `pos`, `type`, `state`, `gid`, `starttime`, `endtime`, `pre_level`, `pre_color`, `cur_level`, `cur_color`)
        */
        if (pc->m_train_queue.size())
        {
            json_spirit::Array aq;
            for (std::vector<generalTrainQue>::iterator it = pc->m_train_queue.begin(); it != pc->m_train_queue.end(); ++it)
            {
                json_spirit::Object q;
                q.push_back( Pair("pos", it->pos) );
                q.push_back( Pair("type", it->type) );
                q.push_back( Pair("state", it->state) );
                q.push_back( Pair("gid", it->general.get() ? it->general->m_gid : 0) );
                q.push_back( Pair("pre_level", it->general.get() ? it->general->pre_level : 0) );
                q.push_back( Pair("pre_color", it->general.get() ? it->general->pre_color : 0) );
                q.push_back( Pair("cur_level", it->general.get() ? it->general->cur_level : 0) );
                q.push_back( Pair("cur_color", it->general.get() ? it->general->cur_color : 0) );
            }
        }

        /*������Ϣ
        `char_treasures` (`cid`, `tid`, `nums`)
        */

        /*VIP�����Ϣ
        `char_vip_present` (`cid`, `vip_id`)
        */

        /*������Ϣ
        `char_zhens` (`cid`, `type`, `level`, `name`, `pos1`, `pos2`, `pos3`, `pos4`, `pos5`, `pos6`, `pos7`, `pos8`, `pos9`)
        */

        /*��ֵ��¼
        `pay_list` (`pay_id`, `pay_orderno`, `qid`, `union_id`, `server_id`, `pay_ip`, `pay_time`,
        `pay_src_money`, `pay_net_money`, `pay_type`, `pay_result`, `pay_endtime`, `pay_reason`)
        */

        /*�̵깺���¼*/

        /*ͨ����Ϣ*/
    }
    q.get_result("select  from charactors as ");
}

/* ��һ���ļ��е����ɫ���� */



