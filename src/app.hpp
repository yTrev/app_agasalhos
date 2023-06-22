#include <imgui.h>
#include <sqlite_orm/sqlite_orm.h>

#include <ctime>
#include <iostream>
#include <memory>
#include <regex>
#include <string>

#include "app_base.hpp"

using namespace sqlite_orm;

struct Doador {
  int id;
  std::string nome;
  std::string telefone;
};

struct Doacao {
  int id;
  std::string data;
  std::string tamanho;
  std::string condicao;
  std::string status;
  std::string descricao;
  std::unique_ptr<int> id_doador;
};

struct TextFilters {
  static int filter_phone(ImGuiInputTextCallbackData* data) {
    // Os únicos caracteres permitidos serão números, espaço, parênteses
    // e hífen
    if (data->EventChar < 48 || data->EventChar > 57) {
      if (data->EventChar != 32 && data->EventChar != 40 &&
          data->EventChar != 41 && data->EventChar != 45)
        return 1;
    }

    return 0;
  };

  static int filter_date(ImGuiInputTextCallbackData* data) {
    // Os únicos caracteres permitidos serão número e /
    if (data->EventChar < 48 || data->EventChar > 57) {
      if (data->EventChar != 47) return 1;
    }

    return 0;
  };
};

void formatPhoneNumber(char* numero) {
  // Remove todos os caracteres não numéricos do número
  std::string clean_phone = std::regex_replace(numero, std::regex("\\D+"), "");

  // Verifica se o número é válido
  if (clean_phone.length() < 10 || clean_phone.length() > 11) {
    strcpy_s(numero, 16, "");

    return;
  }

  // Aplica o formato adequado
  std::string new_phone;
  if (clean_phone.length() == 10) {
    new_phone = std::regex_replace(
        clean_phone, std::regex("(\\d{2})(\\d{4})(\\d{4})"), "($1) $2-$3");
  } else {
    new_phone = std::regex_replace(
        clean_phone, std::regex("(\\d{2})(\\d{5})(\\d{4})"), "($1) $2-$3");
  }

  // Copia o número formatado para o parâmetro original
  strncpy_s(numero, 16, new_phone.c_str(), new_phone.length() + 1);
}

void formatDate(char* data) {
  // Remove todos os caracteres não numéricos da data
  std::string dataLimpa = std::regex_replace(data, std::regex("\\D+"), "");

  // Verifica se a data é válida
  if (dataLimpa.length() != 8) {
    strcpy_s(data, 16, "");

    return;
  }

  // Aplica o formato "DD/MM/YYYY"
  snprintf(data, 11, "%s/%s/%s", dataLimpa.substr(0, 2).c_str(),
           dataLimpa.substr(2, 2).c_str(), dataLimpa.substr(4).c_str());
}

bool validatePhone(const char* phone) {
  std::regex reg("\\(\\d{2}\\)\\s\\d{5}-\\d{4}");

  return std::regex_match(phone, reg);
}

bool validateDate(const char* date) {
  std::regex reg("\\d{2}/\\d{2}/\\d{4}");

  return std::regex_match(date, reg);
}

inline auto initStorage(const std::string& path) {
  return make_storage(
      path,

      make_table("doador",
                 make_column("id", &Doador::id, primary_key().autoincrement()),
                 make_column("nome", &Doador::nome),
                 make_column("telefone", &Doador::telefone)),
      make_table("doacao",
                 make_column("id", &Doacao::id, primary_key().autoincrement()),
                 make_column("data", &Doacao::data),
                 make_column("tamanho", &Doacao::tamanho),
                 make_column("condicao", &Doacao::condicao),
                 make_column("status", &Doacao::status),
                 make_column("descricao", &Doacao::descricao),
                 make_column("id_doador", &Doacao::id_doador),
                 foreign_key(&Doacao::id_doador).references(&Doador::id)));
};

using Storage = decltype(initStorage(""));

class App : public AppBase<App> {
 public:
  App(){};
  virtual ~App() = default;

  void StartUp() {
    stor = std::make_unique<Storage>(initStorage("agasalhos.sqlite"));
    stor->sync_schema();
  }

  void Update() {
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGui::Begin("Doação de agasalhos", NULL,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoBackground);

    if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_FittingPolicyScroll)) {
      if (ImGui::BeginTabItem("Cadastrar")) {
        ImGui::Text("Dados pessoais");
        ImGui::Separator();

        static char nome[256];
        ImGui::InputTextWithHint("##nome", "Nome", nome, 256);
        ImGui::SameLine();
        ImGui::Text("Nome*");

        static char telefone[16];
        if (ImGui::InputTextWithHint("##telefone", "(__) _____-____", telefone,
                                     16,
                                     ImGuiInputTextFlags_CallbackCharFilter |
                                         ImGuiInputTextFlags_EnterReturnsTrue,
                                     TextFilters::filter_phone)) {
          // Formatar o número de telefone, usando regex
          formatPhoneNumber(telefone);
        };

        ImGui::SameLine();
        ImGui::Text("Número de Telefone*");

        ImGui::Text("Informações do agasalho");
        ImGui::Separator();

        // Pegar a data atual
        static char data[16];

        if (ImGui::InputTextWithHint("##data", "__/__/____", data, 16,
                                     ImGuiInputTextFlags_CallbackCharFilter |
                                         ImGuiInputTextFlags_EnterReturnsTrue,
                                     TextFilters::filter_date)) {
          formatDate(data);
        };

        ImGui::SameLine();
        ImGui::Text("Data da doação*");

        // Cria um combo para selecionar o tamanho do agasalho
        const char* tamanhos[] = {"P", "M", "G", "GG", "XG"};
        static const char* tamanho_selecionado = "M";

        if (ImGui::BeginCombo("##tamanhos", tamanho_selecionado)) {
          for (int n = 0; n < IM_ARRAYSIZE(tamanhos); n++) {
            bool is_selected = (tamanho_selecionado == tamanhos[n]);
            if (ImGui::Selectable(tamanhos[n], is_selected))
              tamanho_selecionado = tamanhos[n];

            if (is_selected) ImGui::SetItemDefaultFocus();
          }

          ImGui::EndCombo();
        };

        ImGui::SameLine();
        ImGui::Text("Tamanho*");

        // Cria um combo para selecionar a condição do agasalho
        const char* condicoes[] = {"Novo", "Semi-novo", "Usado"};
        static const char* condicao_selecionada = "Novo";

        if (ImGui::BeginCombo("##condicao", condicao_selecionada)) {
          for (int n = 0; n < IM_ARRAYSIZE(condicoes); n++) {
            bool is_selected = (condicao_selecionada == condicoes[n]);
            if (ImGui::Selectable(condicoes[n], is_selected))
              condicao_selecionada = condicoes[n];

            if (is_selected) ImGui::SetItemDefaultFocus();
          }

          ImGui::EndCombo();
        };

        ImGui::SameLine();
        ImGui::Text("Condição*");

        static char descricao[256];
        ImGui::InputTextWithHint("##descricao", "Descrição", descricao, 256);
        ImGui::SameLine();
        ImGui::Text("Descrição");

        if (ImGui::BeginPopup(
                "Campos obrigatórios",
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
          ImGui::Text("Os campos com * são obrigatórios");
          ImGui::Separator();

          if (ImGui::Button("OK", ImVec2(60, 0))) {
            ImGui::CloseCurrentPopup();
          }

          ImGui::EndPopup();
        };

        if (ImGui::Button("Registrar doação")) {
          // Verificar se os campos obrigatórios foram preenchidos
          if (strlen(nome) == 0 || !validatePhone(telefone) ||
              !validateDate(data)) {
            ImGui::OpenPopup("Campos obrigatórios");

          } else {
            // Registrar nova doação, e criar um novo doador se necessário
            // Verificar se um doador com o mesmo email já existe
            auto doadores_com_mesmo_email = stor->get_all<Doador>(
                where(c(&Doador::telefone) == std::string(telefone)));

            int doador_id = -1;

            if (doadores_com_mesmo_email.empty()) {
              // O doador não existe, então vamos criar um novo
              Doador doador{-1, nome, telefone};
              doador_id = stor->insert(doador);
            } else {
              // O doador já existe, então vamos usar o doador existente
              doador_id = doadores_com_mesmo_email[0].id;
            }

            Doacao doacao{
                -1,
                data,
                tamanho_selecionado,
                condicao_selecionada,
                "Disponível",
                descricao,
                std::make_unique<int>(doador_id),
            };

            stor->insert(doacao);

            // Limpar variáveis
            *nome = 0;
            *telefone = 0;
            *data = 0;
            tamanho_selecionado = "M";
            condicao_selecionada = "Novo";
          }
        };

        ImGui::EndTabItem();
      }

      // Nessa aba, o usuário pode ver os agasalhos disponíveis para doação
      // e marcar como doado aqueles que foram doados
      if (ImGui::BeginTabItem("Estoque de agasalhos")) {
        static bool apenas_disponiveis = true;

        ImGui::Checkbox("##agasalhos_disponiveis", &apenas_disponiveis);
        ImGui::SameLine();
        ImGui::Text("Exibir apenas agasalhos disponíveis");

        const auto rows = stor->select(object<Doacao>());
        ImGui::Columns(6, "doacoes");
        ImGui::Separator();
        ImGui::Text("Data da doação");
        ImGui::NextColumn();
        ImGui::Text("Tamanho");
        ImGui::NextColumn();
        ImGui::Text("Condição");
        ImGui::NextColumn();
        ImGui::Text("Descrição");
        ImGui::NextColumn();
        ImGui::Text("Doador");
        ImGui::NextColumn();
        ImGui::Text("Status");
        ImGui::NextColumn();

        for (auto& doacao : rows) {
          if (apenas_disponiveis && doacao.status == "Doado") {
            continue;
          };

          ImGui::PushID(doacao.id);

          ImGui::Text(doacao.data.c_str());
          ImGui::NextColumn();
          ImGui::Text(doacao.tamanho.c_str());
          ImGui::NextColumn();
          ImGui::Text(doacao.condicao.c_str());
          ImGui::NextColumn();

          // Caso a descrição seja vazia, mostrar "Nenhuma descrição"
          if (doacao.descricao.length() > 0) {
            ImGui::Text(doacao.descricao.c_str());
          } else {
            ImGui::Text("Nenhuma descrição");
          }

          ImGui::NextColumn();

          auto doador = stor->get<Doador>(*doacao.id_doador);

          ImGui::Text(doador.nome.c_str());
          ImGui::NextColumn();

          const char* row_status = doacao.status.c_str();
          const char* status[] = {"Disponível", "Doado"};

          // Cria um combo para permitir alterar o status da doação
          if (ImGui::BeginCombo("##status", row_status)) {
            for (int n = 0; n < IM_ARRAYSIZE(status); n++) {
              bool is_selected = (row_status == status[n]);
              if (ImGui::Selectable(status[n], is_selected)) {
                Doacao temp_doacao = stor->get<Doacao>(doacao.id);
                temp_doacao.status = status[n];

                // Atualiza o status da doação no banco de dados
                stor->update(temp_doacao);
              };

              if (is_selected) ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
          };

          ImGui::SameLine();

          // Abrir Popup para confirmar a exclusão da doação
          if (ImGui::SmallButton("X")) {
            ImGui::OpenPopup("Deletar?");
          };

          // Centralizar o popup
          ImVec2 center = ImGui::GetMainViewport()->GetCenter();
          ImGui::SetNextWindowPos(center, ImGuiCond_Appearing,
                                  ImVec2(0.5f, 0.5f));

          // Popup para confirmar a exclusão da doação
          if (ImGui::BeginPopupModal("Deletar?", NULL,
                                     ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Tem certeza que deseja deletar a doação?");

            if (ImGui::Button("OK", ImVec2(120, 0))) {
              stor->remove<Doacao>(doacao.id);

              ImGui::CloseCurrentPopup();
            };

            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();

            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
              ImGui::CloseCurrentPopup();
            };

            ImGui::EndPopup();
          };

          ImGui::PopID();

          ImGui::NextColumn();
        };

        ImGui::Columns(1);

        ImGui::EndTabItem();
      };

      // Nessa aba, o usuário pode ver os doadores e quantidade de doações
      if (ImGui::BeginTabItem("Doadores")) {
        const auto rows = stor->select(object<Doador>());

        ImGui::Columns(3, "doadores");
        ImGui::Separator();
        ImGui::Text("Nome");
        ImGui::NextColumn();
        ImGui::Text("Telefone");
        ImGui::NextColumn();
        ImGui::Text("Total de doações");
        ImGui::NextColumn();

        for (auto& doador : rows) {
          ImGui::Text(doador.nome.c_str());
          ImGui::NextColumn();
          ImGui::Text(doador.telefone.c_str());
          ImGui::NextColumn();

          // Conta quantas doações o doador fez
          const auto doacoes =
              stor->count<Doacao>(where(c(&Doacao::id_doador) == doador.id));

          ImGui::Text("%d", doacoes);
          ImGui::NextColumn();
        };

        ImGui::Columns(1);

        ImGui::EndTabItem();
      };

      // Apenas algumas informações sobre o projeto
      if (ImGui::BeginTabItem("Sobre")) {
        ImGui::Text("Informações sobre o projeto");
        ImGui::Separator();

        ImGui::Text(
            "Projeto foi feito utilizando a biblioteca ImGui para a "
            "interface "
            "gráfica e SQLite + sqlite_rom para o banco de dados.");

        ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
    };

    ImGui::End();
  }

 private:
  std::unique_ptr<Storage> stor;
};
