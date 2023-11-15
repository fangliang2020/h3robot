#ifndef JYBOT_CONTROL_PID_H
#define JYBOT_CONTROL_PID_H


namespace control
{
    class PID
    {
        public:
            PID();
            PID(const float &kp, const float &ki, const float &kd);
            void reset();
            void set_param(const float &kp, const float &ki, const float &kd);
            void update_pid(const float &p, const float &i, const float &d);
            void update_pid();
            float calc_pid();
            float calc_pi();
            float p();
            float d();
            float i();
            float err();
            float preErr();
            void update_err(const float &err);

        private:
            float kp_, ki_, kd_;
            float p_, i_, d_;
            float err_, pre_err_;
            bool init_err_;
    };
}




#endif